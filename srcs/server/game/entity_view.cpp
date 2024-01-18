#include "stdafx.h"

#include "utils.h"
#include "char.h"
#include "sectree_manager.h"
#include "config.h"
#include "desc.h"

void CEntity::ViewCleanup()
{
	auto it = m_map_view.begin();

	while (it != m_map_view.end())
	{
		LPENTITY entity = it->first;
		++it;

		entity->ViewRemove(this, false);
	}

	m_map_view.clear();
}

void CEntity::ViewReencode()
{
	if (m_bIsObserver)
		return;

	EncodeRemovePacket(this);
	EncodeInsertPacket(this);

	auto it = m_map_view.begin();

	while (it != m_map_view.end())
	{
		LPENTITY entity = (it++)->first;

		EncodeRemovePacket(entity);
		if (!m_bIsObserver)
			EncodeInsertPacket(entity);

		if (!entity->m_bIsObserver)
			entity->EncodeInsertPacket(this);
	}

}

void CEntity::ViewInsert(LPENTITY entity, bool recursive)
{
	if (entity == NULL || this == entity)
		return;

	const auto it = m_map_view.find(entity);

	if (m_map_view.end() != it)
	{
		it->second = m_iViewAge;
		return;
	}

	m_map_view.emplace(entity, m_iViewAge);

	if (!entity->m_bIsObserver)
		entity->EncodeInsertPacket(this);

	if (recursive)
		entity->ViewInsert(this, false);
}

void CEntity::ViewRemove(LPENTITY entity, bool recursive)
{
	if (entity == NULL) {
		return;
	}

	ENTITY_MAP::iterator it = m_map_view.find(entity);

	if (it == m_map_view.end())
		return;

	m_map_view.erase(it);

	if (!entity->m_bIsObserver)
		entity->EncodeRemovePacket(this);

	if (recursive)
		entity->ViewRemove(this, false);
}

class CFuncViewInsert
{
	private:
		int dwViewRange;

	public:
		LPENTITY m_me;

		CFuncViewInsert(LPENTITY ent) :
			dwViewRange(VIEW_RANGE + VIEW_BONUS_RANGE),
			m_me(ent)
		{
		}

		void operator () (LPENTITY ent)
		{
			if (!ent)
				return;

			// NOTE: no item to item insert
			if (m_me->IsType(ENTITY_ITEM) && ent->IsType(ENTITY_ITEM))
				return;

			// NOTE: no item to NPC insert
			if (m_me->IsType(ENTITY_ITEM) && ent->IsType(ENTITY_CHARACTER) && !ent->GetDesc())
				return;

			// NOTE: no NPC to item insert
			if (m_me->IsType(ENTITY_CHARACTER) && !m_me->GetDesc() && ent->IsType(ENTITY_ITEM))
				return;

			if (m_me->IsType(ENTITY_NEWSHOPS) && ent->IsType(ENTITY_NEWSHOPS))
				return;

			if (m_me->IsType(ENTITY_ITEM) && ent->IsType(ENTITY_NEWSHOPS))
				return;

			if (m_me->IsType(ENTITY_NEWSHOPS) && ent->IsType(ENTITY_ITEM))
				return;

			if (m_me->IsType(ENTITY_NEWSHOPS) && ent->IsType(ENTITY_CHARACTER) && !ent->GetDesc())
				return;

			if (m_me->IsType(ENTITY_CHARACTER) && !m_me->GetDesc() && ent->IsType(ENTITY_NEWSHOPS))
				return;
			
			/*
			if (ent->IsType(ENTITY_NEWSHOPS))
			{
				if (DISTANCE_APPROX(ent->GetX() - m_me->GetX(), ent->GetY() - m_me->GetY()) > 1500)
					return;
			}
			*/

			if (ent->IsType(ENTITY_CHARACTER) && m_me->IsType(ENTITY_CHARACTER))
			{
				const auto chMe = dynamic_cast<LPCHARACTER>(m_me);
				const auto chEnt = dynamic_cast<LPCHARACTER>(ent);

				if (chMe->IsPC() && chEnt->IsPC())
				{
					if ((!chMe->IsGM()) && (!chEnt->IsGM()))
					{
						if (chMe->GetMapIndex() == 113)
							return;
					}
				}
			}

			// ������Ʈ�� �ƴ� ���� �Ÿ��� ����Ͽ� �Ÿ��� �ָ� �߰����� �ʴ´�.
			if (!ent->IsType(ENTITY_OBJECT))
				if (DISTANCE_APPROX(ent->GetX() - m_me->GetX(), ent->GetY() - m_me->GetY()) > dwViewRange)
					return;

			// ���� ��� �߰�
			m_me->ViewInsert(ent);

			// �Ѵ� ĳ���͸�
			if (ent->IsType(ENTITY_CHARACTER) && m_me->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER chMe = (LPCHARACTER) m_me;
				LPCHARACTER chEnt = (LPCHARACTER) ent;

				// ����� NPC�� StateMachine�� Ų��.
				if (chMe->IsPC() && !chEnt->IsPC() && !chEnt->IsWarp() && !chEnt->IsGoto())
					chEnt->StartStateMachine();
			}
		}
};

void CEntity::UpdateSectree()
{
	if (!GetSectree())
	{
		if (IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER tch = (LPCHARACTER) this;
			if (tch->GetDesc() && tch->GetDesc()->IsPhase(PHASE_GAME))
				sys_err("null sectree name: %s %d %d",  tch->GetName(), GetX(), GetY());
		}

		return;
	}

	++m_iViewAge;

	CFuncViewInsert f(this); // ���� ��Ʈ���� �ִ� ����鿡�� �߰�
	GetSectree()->ForEachAround(f);

	ENTITY_MAP::iterator it, this_it;

	//
	// m_map_view���� �ʿ� ���� �༮�� �����
	//
	if (m_bObserverModeChange)
	{
		if (m_bIsObserver)
		{
			it = m_map_view.begin();

			while (it != m_map_view.end())
			{
				this_it = it++;
				if (this_it->second < m_iViewAge)
				{
					LPENTITY ent = this_it->first;

					// ���� ���� ������ �����.
					ent->EncodeRemovePacket(this);
					m_map_view.erase(this_it);

					// ���� ���� ���� �����.
					ent->ViewRemove(this, false);
				}
				else
				{

					LPENTITY ent = this_it->first;

					// ���� ���� ������ �����.
					//ent->EncodeRemovePacket(this);
					//m_map_view.erase(this_it);

					// ���� ���� ���� �����.
					//ent->ViewRemove(this, false);
					EncodeRemovePacket(ent);
				}
			}
		}
		else
		{
			it = m_map_view.begin();

			while (it != m_map_view.end())
			{
				this_it = it++;

				if (this_it->second < m_iViewAge)
				{
					LPENTITY ent = this_it->first;

					// ���� ���� ������ �����.
					ent->EncodeRemovePacket(this);
					m_map_view.erase(this_it);

					// ���� ���� ���� �����.
					ent->ViewRemove(this, false);
				}
				else
				{
					LPENTITY ent = this_it->first;
					ent->EncodeInsertPacket(this);
					EncodeInsertPacket(ent);

					ent->ViewInsert(this, true);
				}
			}
		}

		m_bObserverModeChange = false;
	}
	else
	{
		if (!m_bIsObserver)
		{
			it = m_map_view.begin();

			while (it != m_map_view.end())
			{
				this_it = it++;

				if (this_it->second < m_iViewAge)
				{
					LPENTITY ent = this_it->first;

					// ���� ���� ������ �����.
					ent->EncodeRemovePacket(this);
					m_map_view.erase(this_it);

					// ���� ���� ���� �����.
					ent->ViewRemove(this, false);
				}
			}
		}
	}
}

