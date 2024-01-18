#include "stdafx.h"
#include "config.h"
#include "utils.h"
#include "vector.h"
#include "char.h"
#include "sectree_manager.h"
#include "char_manager.h"
#include "mob_manager.h"
#include "PetSystem.h"
#include "../common/VnumHelper.h"
#include "packet.h"
#include "item_manager.h"
#include "item.h"
#include "desc.h"

EVENTINFO(petsystem_event_info)
{
	CPetSystem* pPetSystem;
};

// PetSystem�� update ���ִ� event.
// PetSystem�� CHRACTER_MANAGER���� ���� FSM���� update ���ִ� ���� chracters�� �޸�,
// Owner�� STATE�� update �� �� _UpdateFollowAI �Լ��� update ���ش�.
// �׷��� owner�� state�� update�� CHRACTER_MANAGER���� ���ֱ� ������,
// petsystem�� update�ϴٰ� pet�� unsummon�ϴ� �κп��� ������ �����.
// (CHRACTER_MANAGER���� update �ϸ� chracter destroy�� pending�Ǿ�, CPetSystem������ dangling �����͸� ������ �ְ� �ȴ�.)
// ���� PetSystem�� ������Ʈ ���ִ� event�� �߻���Ŵ.
EVENTFUNC(petsystem_update_event)
{
	petsystem_event_info* info = dynamic_cast<petsystem_event_info*>( event->info );
	if (info == nullptr)
	{
		sys_err("petsystem_update_event> <Factor> Null pointer");
		return 0;
	}

	CPetSystem*	pPetSystem = info->pPetSystem;

	if (nullptr == pPetSystem)
	{
		return 0;
	}


	pPetSystem->Update(0);
	// 0.25�ʸ��� ����.
	return PASSES_PER_SEC(1) / 4;
}

/// NOTE: 1ĳ���Ͱ� ��� ���� ���� �� �ִ��� ����... ĳ���͸��� ������ �ٸ��� �ҰŶ�� ������ �ֵ... ��..
/// ���� �� �ִ� ������ ���ÿ� ��ȯ�� �� �ִ� ������ Ʋ�� �� �ִµ� �̷��� ��ȹ ������ �ϴ� ����
const float PET_COUNT_LIMIT = 3;

///////////////////////////////////////////////////////////////////////////////////////
//  CPetActor
///////////////////////////////////////////////////////////////////////////////////////

CPetActor::CPetActor(LPCHARACTER owner, DWORD vnum, DWORD options)
{
    m_bUnderDestroy = false;

	m_dwRealVnum = vnum;
	m_dwVID = 0;
	m_dwOptions = options;
	m_dwLastActionTime = 0;

	m_pkChar = 0;
	m_pkOwner = owner;

	m_originalMoveSpeed = 0;

	m_dwSummonItemVID = 0;
	m_dwSummonItemVnum = 0;
}

CPetActor::~CPetActor()
{
    m_bUnderDestroy = true;

	this->Unsummon();

	m_pkOwner = 0;
}

void CPetActor::SetName(const char* name)
{
	std::string petName = m_pkOwner->GetName();
	petName += "'s Pet";

	if (true == IsSummoned())
		m_pkChar->SetName(petName);

	m_name = petName;
}

bool CPetActor::Mount()
{
	if (0 == m_pkOwner)
		return false;

	if (true == HasOption(EPetOption_Mountable))
		m_pkOwner->MountVnum(m_dwRealVnum);

	return m_pkOwner->GetMountVnum() == m_dwRealVnum;;
}

void CPetActor::Unmount()
{
	if (0 == m_pkOwner)
		return;

	if (m_pkOwner->IsHorseRiding())
		m_pkOwner->StopRiding();
}

void CPetActor::Unsummon()
{
	if (true == this->IsSummoned())
	{
		LPITEM pSummonItem = ITEM_MANAGER::instance().FindByVID(this->GetSummonItemVID());
		if (pSummonItem) {
			pSummonItem->SetSocket(2, false);
			pSummonItem->Lock(false);
		}
		
		// ���� ����
		this->ClearBuff();
		this->SetSummonItem(nullptr);

        if (m_bUnderDestroy == false && m_pkOwner)
        {
            m_pkOwner->ComputePoints();
        }

		if (nullptr != m_pkChar)
			M2_DESTROY_CHARACTER(m_pkChar);

		m_pkChar = 0;
		m_dwVID = 0;
	}
}

DWORD CPetActor::Summon(const char* petName, LPITEM pSummonItem, bool bSpawnFar)
{
	long x = m_pkOwner->GetX();
	long y = m_pkOwner->GetY();
	long z = m_pkOwner->GetZ();

	if (true == bSpawnFar)
	{
		x += (number(0, 1) * 2 - 1) * number(2000, 2500);
		y += (number(0, 1) * 2 - 1) * number(2000, 2500);
	}
	else
	{
		x += number(-100, 100);
		y += number(-100, 100);
	}

	if (0 != m_pkChar)
	{
		m_pkChar->Show(m_pkOwner->GetMapIndex(), x, y);
		m_dwVID = m_pkChar->GetVID();
		m_pkChar->SetObserverMode(m_pkOwner->IsObserverMode());
		return m_dwVID;
	}

	m_dwVnum = m_dwRealVnum;

#if defined(ENABLE_PETSKIN)
	uint32_t skinVnum = m_pkOwner->GetPoint(POINT_PETSKIN);
	if (skinVnum > 0) {
#if defined(ENABLE_HIDE_COSTUME_SYSTEM)
		if (m_pkOwner->GetPartStatus(HIDE_PETSKIN) != 1) {
			m_dwVnum = skinVnum;
		}
#else
		m_dwVnum = skinVnum;
#endif
	}
#endif

	m_pkChar = CHARACTER_MANAGER::instance().SpawnMob(
				m_dwVnum,
				m_pkOwner->GetMapIndex(),
				x, y, z,
				false, (int)(m_pkOwner->GetRotation()+180), false);

	if (0 == m_pkChar)
	{
		sys_err("[CPetSystem::Summon] Failed to summon the pet. (vnum: %d)", m_dwVnum);
		return 0;
	}

	m_pkChar->SetPet();

//	m_pkOwner->DetailLog();
//	m_pkChar->DetailLog();

	//���� ������ ������ ������ ������.
	m_pkChar->SetEmpire(m_pkOwner->GetEmpire());

	m_dwVID = m_pkChar->GetVID();

	this->SetName("");

	// SetSummonItem(pSummonItem)�� �θ� �Ŀ� ComputePoints�� �θ��� ���� �����.
	this->SetSummonItem(pSummonItem);
	m_pkOwner->ComputePoints();
	m_pkChar->Show(m_pkOwner->GetMapIndex(), x, y, z);
	m_pkChar->SetObserverMode(m_pkOwner->IsObserverMode());
	pSummonItem->SetSocket(2, true);
	pSummonItem->Lock(true);
#ifdef ENABLE_RECALL
	const CAffect* pAffect = m_pkOwner->FindAffect(AFFECT_RECALL1);
	if (pAffect) {
		m_pkOwner->RemoveAffect(const_cast<CAffect*>(pAffect));
	}
	
	m_pkOwner->AddAffect(AFFECT_RECALL1, APPLY_NONE, 0, pSummonItem->GetID(), INFINITE_AFFECT_DURATION, 0, true, false);
#endif
	return m_dwVID;
}

bool CPetActor::_UpdatAloneActionAI(float fMinDist, float fMaxDist)
{
	float fDist = number(fMinDist, fMaxDist);
	float r = (float)number (0, 359);
	float dest_x = GetOwner()->GetX() + fDist * cos(r);
	float dest_y = GetOwner()->GetY() + fDist * sin(r);

	//m_pkChar->SetRotation(number(0, 359));        // ������ �������� ����

	//GetDeltaByDegree(m_pkChar->GetRotation(), fDist, &fx, &fy);

	// ������ ���� �Ӽ� üũ; ���� ��ġ�� �߰� ��ġ�� �������ٸ� ���� �ʴ´�.
	//if (!(SECTREE_MANAGER::instance().IsMovablePosition(m_pkChar->GetMapIndex(), m_pkChar->GetX() + (int) fx, m_pkChar->GetY() + (int) fy)
	//			&& SECTREE_MANAGER::instance().IsMovablePosition(m_pkChar->GetMapIndex(), m_pkChar->GetX() + (int) fx/2, m_pkChar->GetY() + (int) fy/2)))
	//	return true;

	m_pkChar->SetNowWalking(true);

	//if (m_pkChar->Goto(m_pkChar->GetX() + (int) fx, m_pkChar->GetY() + (int) fy))
	//	m_pkChar->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
	if (!m_pkChar->IsStateMove() && m_pkChar->Goto(dest_x, dest_y))
		m_pkChar->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);

	m_dwLastActionTime = get_dword_time();

	return true;
}

// char_state.cpp StateHorse�Լ� �׳� C&P -_-;
bool CPetActor::_UpdateFollowAI()
{
	if (0 == m_pkChar->m_pkMobData)
	{
		//sys_err("[CPetActor::_UpdateFollowAI] m_pkChar->m_pkMobData is nullptr");
		return false;
	}

	// NOTE: ĳ����(��)�� ���� �̵� �ӵ��� �˾ƾ� �ϴµ�, �ش� ��(m_pkChar->m_pkMobData->m_table.sMovingSpeed)�� ���������� �����ؼ� �˾Ƴ� ���� ������
	// m_pkChar->m_pkMobData ���� invalid�� ��찡 ���� �߻���. ���� �ð������ ������ ������ �ľ��ϰ� �ϴ��� m_pkChar->m_pkMobData ���� �ƿ� ������� �ʵ��� ��.
	// ���⼭ �Ź� �˻��ϴ� ������ ���� �ʱ�ȭ �� �� ���� ���� ����� �������� ��쵵 ����.. -_-;; �ФФФФФФФФ�
	if (0 == m_originalMoveSpeed)
	{
		const CMob* mobData = CMobManager::Instance().Get(m_dwRealVnum);

		if (0 != mobData)
			m_originalMoveSpeed = mobData->m_table.sMovingSpeed;
	}
	float	START_FOLLOW_DISTANCE = 300.0f;		// �� �Ÿ� �̻� �������� �Ѿư��� ������
	float	START_RUN_DISTANCE = 900.0f;		// �� �Ÿ� �̻� �������� �پ �Ѿư�.

	float	RESPAWN_DISTANCE = 4500.f;			// �� �Ÿ� �̻� �־����� ���� ������ ��ȯ��.
	int		APPROACH = 200;						// ���� �Ÿ�

	//bool bDoMoveAlone = true;					// ĳ���Ϳ� ������ ���� �� ȥ�� �������� �����ϰ��� ���� -_-;
	bool bRun = false;							// �پ�� �ϳ�?

	DWORD currentTime = get_dword_time();

	long ownerX = m_pkOwner->GetX();		long ownerY = m_pkOwner->GetY();
	long charX = m_pkChar->GetX();			long charY = m_pkChar->GetY();

	float fDist = DISTANCE_APPROX(charX - ownerX, charY - ownerY);

	if (fDist >= RESPAWN_DISTANCE)
	{
		float fOwnerRot = m_pkOwner->GetRotation() * 3.141592f / 180.f;
		float fx = -APPROACH * cos(fOwnerRot);
		float fy = -APPROACH * sin(fOwnerRot);
		if (m_pkChar->Show(m_pkOwner->GetMapIndex(), ownerX + fx, ownerY + fy))
		{
			return true;
		}
	}


	if (fDist >= START_FOLLOW_DISTANCE)
	{
		if( fDist >= START_RUN_DISTANCE)
		{
			bRun = true;
		}

		m_pkChar->SetNowWalking(!bRun);		// NOTE: �Լ� �̸����� ���ߴ°��� �˾Ҵµ� SetNowWalking(false) �ϸ� �ٴ°���.. -_-;

		Follow(APPROACH);

		m_pkChar->SetLastAttacked(currentTime);
		m_dwLastActionTime = currentTime;
	}
	else
	{
		m_pkChar->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
	}

	return true;
}

bool CPetActor::Update(DWORD deltaTime)
{
	bool bResult = true;

	if ((this->GetSummonItemVID() != 0 && (nullptr == ITEM_MANAGER::instance().FindByVID(this->GetSummonItemVID()) || ITEM_MANAGER::instance().FindByVID(this->GetSummonItemVID())->GetOwner() != this->GetOwner())))
	{
		this->Unsummon();
		return true;
	}

	if (this->IsSummoned() && HasOption(EPetOption_Followable))
		bResult = bResult && this->_UpdateFollowAI();

	return bResult;
}

//NOTE : ����!!! MinDistance�� ũ�� ������ �� ������ŭ�� ��ȭ������ follow���� �ʴ´�,
bool CPetActor::Follow(float fMinDistance)
{
	// ������ ��ġ�� �ٶ���� �Ѵ�.
	if( !m_pkOwner || !m_pkChar)
		return false;

	float fOwnerX = m_pkOwner->GetX();
	float fOwnerY = m_pkOwner->GetY();

	float fPetX = m_pkChar->GetX();
	float fPetY = m_pkChar->GetY();

	float fDist = DISTANCE_SQRT(fOwnerX - fPetX, fOwnerY - fPetY);
	if (fDist <= fMinDistance)
		return false;

	m_pkChar->SetRotationToXY(fOwnerX, fOwnerY);

	float fx, fy;

	float fDistToGo = fDist - fMinDistance;
	GetDeltaByDegree(m_pkChar->GetRotation(), fDistToGo, &fx, &fy);

	if (!m_pkChar->Goto((int)(fPetX+fx+0.5f), (int)(fPetY+fy+0.5f)) )
		return false;

	m_pkChar->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0, 0);

	return true;
}

void CPetActor::SetSummonItem (LPITEM pItem)
{
	if (nullptr == pItem)
	{
		m_dwSummonItemVID = 0;
		m_dwSummonItemVnum = 0;
		return;
	}

	m_dwSummonItemVID = pItem->GetVID();
	m_dwSummonItemVnum = pItem->GetVnum();
}

bool __PetCheckBuff(const CPetActor* pPetActor)
{
	bool bMustHaveBuff = true;
	switch (pPetActor->GetVnum())
	{
		case 34004:
		case 34009:
			if (nullptr == pPetActor->GetOwner()->GetDungeon())
				bMustHaveBuff = false;
		default:
			break;
	}
	return bMustHaveBuff;
}

void CPetActor::GiveBuff()
{
	// ��Ȳ �� ������ ���������� �߻���.
	if (!__PetCheckBuff(this))
		return;
	LPITEM item = ITEM_MANAGER::instance().FindByVID(m_dwSummonItemVID);
	if (nullptr != item)
		item->ModifyPoints(true);
	return ;
}

void CPetActor::ClearBuff()
{
	if (nullptr == m_pkOwner)
		return ;
	TItemTable* item_proto = ITEM_MANAGER::instance().GetTable(m_dwSummonItemVnum);
	if (nullptr == item_proto)
		return;
	if (!__PetCheckBuff(this)) // @fixme129
		return;
	for (int i = 0; i < ITEM_APPLY_MAX_NUM; i++)
	{
		if (item_proto->aApplies[i].bType == APPLY_NONE)
			continue;
		m_pkOwner->ApplyPoint(item_proto->aApplies[i].bType, -item_proto->aApplies[i].lValue);
	}

	return ;
}

///////////////////////////////////////////////////////////////////////////////////////
//  CPetSystem
///////////////////////////////////////////////////////////////////////////////////////

CPetSystem::CPetSystem(LPCHARACTER owner)
{
//	assert(0 != owner && "[CPetSystem::CPetSystem] Invalid owner");

	m_pkOwner = owner;
	m_dwUpdatePeriod = 400;

	m_dwLastUpdateTime = 0;
}

CPetSystem::~CPetSystem()
{
	Destroy();
}

void CPetSystem::Destroy()
{
	for (TPetActorMap::iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CPetActor* petActor = iter->second;

		if (0 != petActor)
		{
			M2_DELETE(petActor);
		}
	}
	event_cancel(&m_pkPetSystemUpdateEvent);
	m_petActorMap.clear();
}

/// �� �ý��� ������Ʈ. ��ϵ� ����� AI ó�� ���� ��.
bool CPetSystem::Update(DWORD deltaTime)
{
	bool bResult = true;

	DWORD currentTime = get_dword_time();

	// CHARACTER_MANAGER���� ĳ���ͷ� Update�� �� �Ű������� �ִ� (Pulse��� �Ǿ��ִ�)���� ���� �����Ӱ��� �ð��������� �˾Ҵµ�
	// ���� �ٸ� ���̶�-_-; ���⿡ �Է����� ������ deltaTime�� �ǹ̰� �����Ф�

	if (m_dwUpdatePeriod > currentTime - m_dwLastUpdateTime)
		return true;

	std::vector <CPetActor*> v_garbageActor;

	for (auto iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CPetActor* petActor = iter->second;

		if (0 != petActor && petActor->IsSummoned())
		{
			LPCHARACTER pPet = petActor->GetCharacter();

			if (nullptr == CHARACTER_MANAGER::instance().Find(pPet->GetVID()))
			{
				v_garbageActor.push_back(petActor);
			}
			else
			{
				bResult = bResult && petActor->Update(deltaTime);
			}
		}
	}
	for (auto it = v_garbageActor.begin(); it != v_garbageActor.end(); it++)
		DeletePet(*it);

	m_dwLastUpdateTime = currentTime;

	return bResult;
}

/// ���� ��Ͽ��� ���� ����
void CPetSystem::DeletePet(DWORD mobVnum)
{
	auto iter = m_petActorMap.find(mobVnum);

	if (m_petActorMap.end() == iter)
	{
		sys_err("[CPetSystem::DeletePet] Can't find pet on my list (VNUM: %d)", mobVnum);
		return;
	}

	CPetActor* petActor = iter->second;

	if (petActor != nullptr)
	{
		sys_err("[CPetSystem::DeletePet] Null Pointer (petActor)");
	}
	else
	{
		M2_DELETE(petActor);
	}

	m_petActorMap.erase(iter);
}

/// ���� ��Ͽ��� ���� ����
void CPetSystem::DeletePet(CPetActor* petActor)
{
	for (auto iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		if (iter->second == petActor)
		{
			M2_DELETE(petActor);
			m_petActorMap.erase(iter);

			return;
		}
	}

	sys_err("[CPetSystem::DeletePet] Can't find petActor(0x%x) on my list(size: %d) ", petActor, m_petActorMap.size());
}

void CPetSystem::Unsummon(DWORD vnum, bool bDeleteFromList)
{
	CPetActor* actor = this->GetByVnum(vnum);

	if (0 == actor)
	{
		sys_err("[CPetSystem::GetByVnum(%d)] Null Pointer (petActor)", vnum);
		return;
	}
	actor->Unsummon();

	if (true == bDeleteFromList)
	{
		this->DeletePet(actor);
	}

	bool bActive = false;
	for (auto it = m_petActorMap.begin(); it != m_petActorMap.end(); it++)
	{
		bActive |= it->second->IsSummoned();
	}
	if (false == bActive)
	{
		event_cancel(&m_pkPetSystemUpdateEvent);
		m_pkPetSystemUpdateEvent = nullptr;
	}
}

void CPetSystem::UnsummonAll()
{
	for (auto iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CPetActor* petActor = iter->second;
		if (petActor != 0)
		{
			if (petActor->IsSummoned()) {
				petActor->Unsummon();
				break;
			}
		}
	}
	
	if (m_pkPetSystemUpdateEvent) {
		event_cancel(&m_pkPetSystemUpdateEvent);
		m_pkPetSystemUpdateEvent = nullptr;
	}
}

CPetActor* CPetSystem::Summon(DWORD mobVnum, LPITEM pSummonItem, const char* petName, bool bSpawnFar, DWORD options)
{
	CPetActor* petActor = this->GetByVnum(mobVnum);

	// ��ϵ� ���� �ƴ϶�� ���� ���� �� ���� ��Ͽ� �����.
	if (0 == petActor)
	{
		petActor = M2_NEW CPetActor(m_pkOwner, mobVnum, options);
		m_petActorMap.insert(std::make_pair(mobVnum, petActor));
	}

	DWORD petVID = petActor->Summon(petName, pSummonItem, bSpawnFar);
	if (!petVID)
		sys_err("[CPetSystem::Summon(%d)] Null Pointer (petVID)", pSummonItem);

	if (nullptr == m_pkPetSystemUpdateEvent)
	{
		petsystem_event_info* info = AllocEventInfo<petsystem_event_info>();

		info->pPetSystem = this;

		m_pkPetSystemUpdateEvent = event_create(petsystem_update_event, info, PASSES_PER_SEC(1) / 4);	// 0.25��
	}

	return petActor;
}


CPetActor* CPetSystem::GetByVID(DWORD vid) const
{
	CPetActor* petActor = 0;

	bool bFound = false;

	for (auto iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		petActor = iter->second;

		if (0 == petActor)
		{
			sys_err("[CPetSystem::GetByVID(%d)] Null Pointer (petActor)", vid);
			continue;
		}

		bFound = petActor->GetVID() == vid;

		if (true == bFound)
			break;
	}

	return bFound ? petActor : 0;
}

/// ��� �� �� �߿��� �־��� �� VNUM�� ���� ���͸� ��ȯ�ϴ� �Լ�.
CPetActor* CPetSystem::GetByVnum(DWORD vnum) const
{
	CPetActor* petActor = 0;

	auto iter = m_petActorMap.find(vnum);

	if (m_petActorMap.end() != iter)
	{
		petActor = iter->second;
	}

	return petActor;
}

size_t CPetSystem::CountSummoned() const
{
	size_t count = 0;

	for (auto iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CPetActor* petActor = iter->second;

		if (0 != petActor)
		{
			if (petActor->IsSummoned())
			{
				++count;
			}		}
	}

	return count;
}

void CPetSystem::RefreshBuff()
{
	for (TPetActorMap::const_iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CPetActor* petActor = iter->second;

		if (0 != petActor)
		{
			if (petActor->IsSummoned())
			{
				petActor->GiveBuff();
			}
		}
	}
}

#if defined(ENABLE_PETSKIN)
void CPetActor::UpdatePetSkin() {
	LPITEM pSummonItem = ITEM_MANAGER::instance().FindByVID(this->GetSummonItemVID());

	if (pSummonItem != nullptr) {
		Unsummon();
		Summon("Noname", pSummonItem, false);
	}
}

void CPetSystem::UpdatePetSkin() {
	for (auto iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter) {
		CPetActor* petActor = iter->second;
		if (petActor != 0) {
			if (petActor->IsSummoned()) {
				petActor->UpdatePetSkin();
			}
		}
	}
}
#endif

void CPetSystem::UpdateObserver(bool bFlag)
{
	if (!m_pkOwner)
		return;

	for (auto iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		const CPetActor* petActor = iter->second;
		if (petActor == nullptr)
			continue;

		const LPCHARACTER petCh = petActor->GetCharacter();
		if (petCh)
			petCh->SetObserverMode(bFlag);
	}
}
