#include "stdafx.h"
#include "config.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "p2p.h"
#include "guild.h"
#include "guild_manager.h"
#include "party.h"
#include "messenger_manager.h"
#include "empire_text_convert.h"
#include "unique_item.h"
#include "affect.h"
#include "locale_service.h"
#include "questmanager.h"
#include "skill.h"
#ifdef ENABLE_WHISPER_ADMIN_SYSTEM
#include "whisper_admin.h"
#endif
#if defined(ENABLE_ULTIMATE_REGEN)
#include "new_mob_timer.h"
#endif
#if defined(ENABLE_OFFLINESHOP_REWORK)
#include "new_offlineshop.h"
#include "new_offlineshop_manager.h"
#endif

////////////////////////////////////////////////////////////////////////////////
// Input Processor
CInputP2P::CInputP2P()
{
	BindPacketInfo(&m_packetInfoGG);
}

void CInputP2P::Login(LPDESC d, const char * c_pData)
{
	P2P_MANAGER::instance().Login(d, (TPacketGGLogin *) c_pData);
}

void CInputP2P::Logout(LPDESC d, const char * c_pData)
{
	TPacketGGLogout * p = (TPacketGGLogout *) c_pData;
	P2P_MANAGER::instance().Logout(p->szName);
}

int CInputP2P::Relay(LPDESC d, const char * c_pData, size_t uiBytes)
{
	TPacketGGRelay * p = (TPacketGGRelay *) c_pData;

	if (uiBytes < sizeof(TPacketGGRelay) + p->lSize)
		return -1;

	if (p->lSize < 0)
	{
		sys_err("invalid packet length %d", p->lSize);
		d->SetPhase(PHASE_CLOSE);
		return -1;
	}

	sys_log(0, "InputP2P::Relay : %s size %d", p->szName, p->lSize);

	LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindPC(p->szName);

	const BYTE* c_pbData = (const BYTE *) (c_pData + sizeof(TPacketGGRelay));

	if (!pkChr)
		return p->lSize;

	if (*c_pbData == HEADER_GC_WHISPER)
	{
		char buf[1024];
		memcpy(buf, c_pbData, MIN(p->lSize, sizeof(buf)));

		TPacketGCWhisper* p2 = (TPacketGCWhisper*) buf;

		if (pkChr->IsBlockMode(BLOCK_WHISPER) && p2->bType != WHISPER_TYPE_TARGET_BLOCKED)
		{
			// �ӼӸ� �ź� ���¿��� �ӼӸ� �ź�.
			d->SetRelay(p2->szNameFrom);

			p2->bType = WHISPER_TYPE_TARGET_BLOCKED;
			p2->wSize = sizeof(TPacketGCWhisper);
			strlcpy(p2->szNameFrom, p->szName, sizeof(p2->szNameFrom));
			d->Packet(p2, sizeof(TPacketGCWhisper));

			return p->lSize;
		}

		// bType ���� 4��Ʈ: Empire ��ȣ
		// bType ���� 4��Ʈ: EWhisperType
		BYTE bToEmpire = (p2->bType >> 4);
		p2->bType = p2->bType & 0x0F;
		if(p2->bType == 0x0F) {
			// �ý��� �޼��� �ӼӸ��� bType�� ������Ʈ���� ��� �����.
			p2->bType = WHISPER_TYPE_SYSTEM;
		} else {
			if (!pkChr->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE))
				if (bToEmpire >= 1 && bToEmpire <= 3 && pkChr->GetEmpire() != bToEmpire)
				{
					ConvertEmpireText(bToEmpire,
							buf + sizeof(TPacketGCWhisper),
							p2->wSize - sizeof(TPacketGCWhisper),
							10+2*pkChr->GetSkillPower(SKILL_LANGUAGE1 + bToEmpire - 1));
				}
		}

		pkChr->GetDesc()->Packet(buf, p->lSize);
	}
	else
		pkChr->GetDesc()->Packet(c_pbData, p->lSize);

	return (p->lSize);
}

#ifdef ENABLE_FULL_NOTICE
int CInputP2P::Notice(LPDESC d, const char * c_pData, size_t uiBytes, bool bBigFont)
#else
int CInputP2P::Notice(LPDESC d, const char * c_pData, size_t uiBytes)
#endif
{
	TPacketGGNotice * p = (TPacketGGNotice *) c_pData;

	if (uiBytes < sizeof(TPacketGGNotice) + p->lSize)
		return -1;

	if (p->lSize < 0)
	{
		sys_err("invalid packet length %d", p->lSize);
		d->SetPhase(PHASE_CLOSE);
		return -1;
	}

	char szBuf[256+1];
	strlcpy(szBuf, c_pData + sizeof(TPacketGGNotice), MIN(p->lSize + 1, sizeof(szBuf)));
#ifdef ENABLE_FULL_NOTICE
	SendNotice(szBuf, bBigFont);
#else
	SendNotice(szBuf);
#endif
	return (p->lSize);
}

int32_t CInputP2P::NoticeNew(LPDESC d, const char * c_pData, size_t uiBytes)
{
	TPacketGGChatNew * p = (TPacketGGChatNew *) c_pData;
	if (uiBytes < sizeof(TPacketGGChatNew) + p->size) {
		return -1;
	}

	if (p->size < 0) {
		d->SetPhase(PHASE_CLOSE);
		return -1;
	}

	char chatbuf[256 + 1];
	strlcpy(chatbuf, c_pData + sizeof(TPacketGGChatNew), p->size + 1);

	SendNoticeNew(p->type, p->empire, p->mapidx, p->idx, chatbuf);
	return p->size;
}

int CInputP2P::Guild(LPDESC d, const char* c_pData, size_t uiBytes)
{
	TPacketGGGuild * p = (TPacketGGGuild *) c_pData;
	uiBytes -= sizeof(TPacketGGGuild);
	c_pData += sizeof(TPacketGGGuild);

	CGuild * g = CGuildManager::instance().FindGuild(p->dwGuild);

	switch (p->bSubHeader)
	{
		case GUILD_SUBHEADER_GG_CHAT:
			{
				if (uiBytes < sizeof(TPacketGGGuildChat)) {
					return -1;
				}

				const auto* pInfo = reinterpret_cast<const TPacketGGGuildChat*>(c_pData);

				if (g) {
					g->P2PChat(pInfo->szText
#if defined(ENABLE_MESSENGER_BLOCK)
					, pInfo->name
#endif
					);
				}

				return sizeof(TPacketGGGuildChat);
			}

		case GUILD_SUBHEADER_GG_SET_MEMBER_COUNT_BONUS:
			{
				if (uiBytes < sizeof(int))
					return -1;

				int iBonus = *((int *) c_pData);
				CGuild* pGuild = CGuildManager::instance().FindGuild(p->dwGuild);
				if (pGuild)
				{
					pGuild->SetMemberCountBonus(iBonus);
				}
				return sizeof(int);
			}
		default:
			sys_err ("UNKNOWN GUILD SUB PACKET");
			break;
	}
	return 0;
}


struct FuncShout {
	const char* m_str;
	uint8_t m_bEmpire;
#if defined(ENABLE_MESSENGER_BLOCK)
	const char* m_name;
#endif

	FuncShout(const char* str, uint8_t bEmpire
#if defined(ENABLE_MESSENGER_BLOCK)
, const char* name
#endif
	) : m_str(str), m_bEmpire(bEmpire)
#if defined(ENABLE_MESSENGER_BLOCK)
, m_name(name)
#endif
	{
	}

	void operator () (LPDESC d) {
		const LPCHARACTER pkChar = d ? d->GetCharacter() : nullptr;

		if (!pkChar) {
			return;
		}

		if (!g_bGlobalShoutEnable && pkChar->GetGMLevel() == GM_PLAYER && d->GetEmpire() != m_bEmpire)
			return;

#if defined(ENABLE_MESSENGER_BLOCK)
		if (MessengerManager::instance().CheckMessengerList(pkChar->GetName(), m_name, SYST_BLOCK)) {
			return;
		}
#endif

		pkChar->ChatPacket(CHAT_TYPE_SHOUT, "%s", m_str);
	}
};

void SendShout(const char* szText, uint8_t bEmpire
#if defined(ENABLE_MESSENGER_BLOCK)
, const char* name
#endif
) {
	const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
	std::for_each(c_ref_set.begin(), c_ref_set.end(), FuncShout(szText, bEmpire
#if defined(ENABLE_MESSENGER_BLOCK)
, name
#endif
	));
}

void CInputP2P::Shout(const char* data) {
	const auto* pInfo = reinterpret_cast<const TPacketGGShout*>(data);
	SendShout(pInfo->szText, pInfo->bEmpire
#if defined(ENABLE_MESSENGER_BLOCK)
, pInfo->name
#endif
	);
}

void CInputP2P::Disconnect(const char * c_pData)
{
	TPacketGGDisconnect * p = (TPacketGGDisconnect *) c_pData;

	LPDESC d = DESC_MANAGER::instance().FindByLoginName(p->szLogin);

	if (!d)
		return;

	if (!d->GetCharacter())
	{
		d->SetPhase(PHASE_CLOSE);
	}
	else
		d->DisconnectOfSameLogin();
}

void CInputP2P::Setup(LPDESC d, const char * c_pData)
{
	TPacketGGSetup * p = (TPacketGGSetup *) c_pData;
	sys_log(0, "P2P: Setup %s:%d", d->GetHostName(), p->wPort);
	d->SetP2P(d->GetHostName(), p->wPort, p->bChannel);
}

void CInputP2P::MessengerAdd(const char * c_pData)
{
	TPacketGGMessenger * p = (TPacketGGMessenger *) c_pData;
	sys_log(0, "P2P: Messenger Add %s %s", p->szAccount, p->szCompanion);
	MessengerManager::instance().__AddToList(p->szAccount, p->szCompanion);
}

void CInputP2P::MessengerRemove(const char * c_pData)
{
	TPacketGGMessenger * p = (TPacketGGMessenger *) c_pData;
	sys_log(0, "P2P: Messenger Remove %s %s", p->szAccount, p->szCompanion);
	MessengerManager::instance().__RemoveFromList(p->szAccount, p->szCompanion);
}

void CInputP2P::FindPosition(LPDESC d, const char* c_pData)
{
	TPacketGGFindPosition* p = (TPacketGGFindPosition*) c_pData;
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(p->dwTargetPID);

#ifdef __CMD_WARP_IN_DUNGEON__
	if (ch)
#else
	if (ch && ch->GetMapIndex() < 10000)
#endif
	{
		TPacketGGWarpCharacter pw;
		pw.header = HEADER_GG_WARP_CHARACTER;
		pw.pid = p->dwFromPID;
		pw.x = ch->GetX();
		pw.y = ch->GetY();
#ifdef __CMD_WARP_IN_DUNGEON__
		pw.mapIndex = (ch->GetMapIndex() < 10000) ? 0 : ch->GetMapIndex();
#endif
		d->Packet(&pw, sizeof(pw));
	}
}

void CInputP2P::WarpCharacter(const char* c_pData)
{
	TPacketGGWarpCharacter* p = (TPacketGGWarpCharacter*) c_pData;
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(p->pid);
#ifdef __CMD_WARP_IN_DUNGEON__
	if (ch)
	{
		ch->WarpSet(p->x, p->y, p->mapIndex);
	}
#else
	if (ch)
	{
		ch->WarpSet(p->x, p->y);
	}
#endif
}

void CInputP2P::GuildWarZoneMapIndex(const char* c_pData)
{
	TPacketGGGuildWarMapIndex * p = (TPacketGGGuildWarMapIndex*) c_pData;
	CGuildManager & gm = CGuildManager::instance();

	sys_log(0, "P2P: GuildWarZoneMapIndex g1(%u) vs g2(%u), mapIndex(%d)", p->dwGuildID1, p->dwGuildID2, p->lMapIndex);

	CGuild * g1 = gm.FindGuild(p->dwGuildID1);
	CGuild * g2 = gm.FindGuild(p->dwGuildID2);

	if (g1 && g2)
	{
		g1->SetGuildWarMapIndex(p->dwGuildID2, p->lMapIndex);
		g2->SetGuildWarMapIndex(p->dwGuildID1, p->lMapIndex);
	}
}

void CInputP2P::Transfer(const char * c_pData)
{
	TPacketGGTransfer * p = (TPacketGGTransfer *) c_pData;

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(p->szName);

	if (ch)
		ch->WarpSet(p->lX, p->lY);
}

void CInputP2P::LoginPing(LPDESC d, const char * c_pData)
{
	TPacketGGLoginPing * p = (TPacketGGLoginPing *) c_pData;

	if (!g_pkAuthMasterDesc) // If I am master, I have to broadcast
		P2P_MANAGER::instance().Send(p, sizeof(TPacketGGLoginPing), d);
}

// BLOCK_CHAT
void CInputP2P::BlockChat(const char * c_pData)
{
	TPacketGGBlockChat * p = (TPacketGGBlockChat *) c_pData;

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(p->szName);

	if (ch)
	{
		sys_log(0, "BLOCK CHAT apply name %s dur %d", p->szName, p->lBlockDuration);
		ch->AddAffect(AFFECT_BLOCK_CHAT, POINT_NONE, 0, AFF_NONE, p->lBlockDuration, 0, true);
	}
	else
	{
		sys_log(0, "BLOCK CHAT fail name %s dur %d", p->szName, p->lBlockDuration);
	}
}
// END_OF_BLOCK_CHAT
//

void CInputP2P::IamAwake(LPDESC d, const char * c_pData)
{
	std::string hostNames;
	P2P_MANAGER::instance().GetP2PHostNames(hostNames);
	sys_log(0, "P2P Awakeness check from %s. My P2P connection number is %d. and details...\n%s", d->GetHostName(), P2P_MANAGER::instance().GetDescCount(), hostNames.c_str());
}

#if defined(ENABLE_OFFLINESHOP_REWORK)
void CInputP2P::OfflineShopNotification(LPDESC d, const char * c_pData)
{
	TPacketGGOfflineShopNotification * p = (TPacketGGOfflineShopNotification *) c_pData;

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(p->ownerId);
	if (!ch)
		return;
	
	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	offlineshop::CShop* pkShop = rManager.GetShopByOwnerID(p->ownerId);

	if (pkShop)
		pkShop->SendNotificationClientPacket(p->vnum, p->price, p->count);
}
#endif

#if defined(ENABLE_MESSENGER_BLOCK)
void CInputP2P::MessengerBlock(const char* data) {
	const auto* pInfo = reinterpret_cast<const TPacketGGMessengerBlock*>(data);

	if (pInfo->bAdd == 0) {
		MessengerManager::instance().__RemoveFromBlockList(pInfo->szAccount, pInfo->szCompanion);
		return;
	}

	MessengerManager::instance().__AddToBlockList(pInfo->szAccount, pInfo->szCompanion);
}
#endif

#if defined(ENABLE_CLOSE_GAMECLIENT_CMD)
void CInputP2P::CloseClient(const char* data)
{
	const auto* pInfo = reinterpret_cast<const TPacketGGCloseClient*>(data);

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(pInfo->szName);
	if (tch) {
		const LPDESC d = tch->GetDesc();
		if (!d) {
			return;
		}

		TPacketEmpty p;

		p.bHeader = HEADER_GC_CLOSECLIENT;

		d->Packet(&p, sizeof(p));

		d->DelayedDisconnect(3);
	}
}
#endif

#if defined(ENABLE_ULTIMATE_REGEN)
void CInputP2P::NewRegen(const char* data) {
	const auto* pInfo = reinterpret_cast<const TGGPacketNewRegen*>(data);

	switch (pInfo->subHeader) {
		case NEW_REGEN_LOAD: {
			char buf[250];
			snprintf(buf, sizeof(buf), "%s/newregen.txt", LocaleService_GetBasePath().c_str());
			CNewMobTimer::Instance().LoadFile(buf);
			CNewMobTimer::Instance().UpdatePlayers();
			sys_log(0, "Reloading New Regen");
			break;
		}
		case NEW_REGEN_REFRESH: {
			CNewMobTimer::Instance().UpdateNewRegen(pInfo->id, pInfo->isAlive, true);
			break;
		}
		default: {
			sys_err("Uknown subHeader (%d)", pInfo->subHeader);
			break;
		}
	}
}
#endif

int CInputP2P::Analyze(LPDESC d, BYTE bHeader, const char * c_pData)
{
	if (test_server)
		sys_log(0, "CInputP2P::Anlayze[Header %d]", bHeader);

	int iExtraLen = 0;

	switch (bHeader)
	{
		case HEADER_GG_SETUP:
			Setup(d, c_pData);
			break;

		case HEADER_GG_LOGIN:
			Login(d, c_pData);
			break;

		case HEADER_GG_LOGOUT:
			Logout(d, c_pData);
			break;

		case HEADER_GG_RELAY:
			if ((iExtraLen = Relay(d, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;
#ifdef ENABLE_FULL_NOTICE
		case HEADER_GG_BIG_NOTICE:
			if ((iExtraLen = Notice(d, c_pData, m_iBufferLeft, true)) < 0)
				return -1;
			break;
#endif
		case HEADER_GG_NOTICE:
			if ((iExtraLen = Notice(d, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

		case HEADER_GG_SHUTDOWN:
			sys_err("Accept shutdown p2p command from %s.", d->GetHostName());
			Shutdown(10);
			break;

		case HEADER_GG_GUILD:
			if ((iExtraLen = Guild(d, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

		case HEADER_GG_SHOUT:
			Shout(c_pData);
			break;

		case HEADER_GG_DISCONNECT:
			Disconnect(c_pData);
			break;

		case HEADER_GG_MESSENGER_ADD:
			MessengerAdd(c_pData);
			break;

		case HEADER_GG_MESSENGER_REMOVE:
			MessengerRemove(c_pData);
			break;
#if defined(ENABLE_MESSENGER_BLOCK)
		case HEADER_GG_MESSENGER_BLOCK: {
			MessengerBlock(c_pData);
			break;
		}
#endif
		case HEADER_GG_FIND_POSITION:
			FindPosition(d, c_pData);
			break;

		case HEADER_GG_WARP_CHARACTER:
			WarpCharacter(c_pData);
			break;

		case HEADER_GG_GUILD_WAR_ZONE_MAP_INDEX:
			GuildWarZoneMapIndex(c_pData);
			break;

		case HEADER_GG_TRANSFER:
			Transfer(c_pData);
			break;

		case HEADER_GG_RELOAD_CRC_LIST:
			LoadValidCRCList();
			break;

		case HEADER_GG_CHECK_CLIENT_VERSION:
			CheckClientVersion();
			break;

		case HEADER_GG_LOGIN_PING:
			LoginPing(d, c_pData);
			break;

		case HEADER_GG_BLOCK_CHAT:
			BlockChat(c_pData);
			break;
#ifdef ENABLE_REWARD_SYSTEM
		case HEADER_GG_REWARD_INFO:
		{
			TPacketGGRewardInfo* data = (TPacketGGRewardInfo*)c_pData;
			CHARACTER_MANAGER::Instance().SetRewardData(data->rewardIndex, data->playerName, false);
		}
		break;
#endif
#ifdef ENABLE_WHISPER_ADMIN_SYSTEM
		case HEADER_GG_WHISPER_SYSTEM:
			if ((iExtraLen = CWhisperAdmin::instance().Whisper(d, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;
#endif
		case HEADER_GG_CHECK_AWAKENESS:
			IamAwake(d, c_pData);
			break;
#if defined(ENABLE_OFFLINESHOP_REWORK)
		case HEADER_GG_OFFLINE_SHOP_NOTIFICATION:
			OfflineShopNotification(d, c_pData);
			break;
#endif
		case HEADER_GG_CHAT_NEW:
			if ((iExtraLen = NoticeNew(d, c_pData, m_iBufferLeft)) < 0) {
				return -1;
			}
			
			break;
#if defined(ENABLE_CLOSE_GAMECLIENT_CMD)
		case HEADER_GG_CLOSECLIENT: {
			CloseClient(c_pData);
			break;
		}
#endif
#if defined(ENABLE_ULTIMATE_REGEN)
		case HEADER_GG_NEW_REGEN: {
			NewRegen(c_pData);
			break;
		}
#endif
		default: {
			break;
		}
	}

	return (iExtraLen);
}
