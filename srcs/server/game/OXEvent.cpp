#include "stdafx.h"
#include "constants.h"
#include "config.h"
#include "questmanager.h"
#include "start_position.h"
#include "packet.h"
#include "buffer_manager.h"
#include "log.h"
#include "char.h"
#include "char_manager.h"
#include "OXEvent.h"
#include "desc.h"

bool COXEventManager::Initialize()
{
	m_timedEvent = NULL;
	m_map_char.clear();
	m_map_attender.clear();
	m_vec_quiz.clear();

	SetStatus(OXEVENT_FINISH);

	return true;
}

void COXEventManager::Destroy()
{
	CloseEvent();

	m_map_char.clear();
	m_map_attender.clear();
	m_vec_quiz.clear();

	SetStatus(OXEVENT_FINISH);
}

OXEventStatus COXEventManager::GetStatus()
{
	BYTE ret = quest::CQuestManager::instance().GetEventFlag("oxevent_status");

	switch (ret)
	{
		case 0 :
			return OXEVENT_FINISH;

		case 1 :
			return OXEVENT_OPEN;

		case 2 :
			return OXEVENT_CLOSE;

		case 3 :
			return OXEVENT_QUIZ;

		default :
			return OXEVENT_ERR;
	}

	return OXEVENT_ERR;
}

void COXEventManager::SetStatus(OXEventStatus status)
{
	BYTE val = 0;

	switch (status)
	{
		case OXEVENT_OPEN :
			val = 1;
			break;

		case OXEVENT_CLOSE :
			val = 2;
			break;

		case OXEVENT_QUIZ :
			val = 3;
			break;

		case OXEVENT_FINISH :
		case OXEVENT_ERR :
		default :
			val = 0;
			break;
	}
	quest::CQuestManager::instance().RequestSetEventFlag("oxevent_status", val);
}

bool COXEventManager::Enter(LPCHARACTER pkChar)
{
	if (GetStatus() == OXEVENT_FINISH)
	{
		sys_log(0, "OXEVENT : map finished. but char enter. %s", pkChar->GetName());
		return false;
	}

	PIXEL_POSITION pos = pkChar->GetXYZ();

	if (pos.x == 896500 && pos.y == 24600)
	{
		return EnterAttender(pkChar);
	}
	else if (pos.x == 896300 && pos.y == 28900)
	{
		return EnterAudience(pkChar);
	}
	else
	{
		sys_log(0, "OXEVENT : wrong pos enter %d %d", pos.x, pos.y);
		return false;
	}

	return false;
}

bool COXEventManager::EnterAttender(LPCHARACTER pkChar)
{
#if defined(ENABLE_RENEWAL_OX)
	if (!pkChar) {
		return false;
	}

	if (!CheckIpAddress(pkChar)) {
		return false;
	}

	if (pkChar->GetGMLevel() == GM_PLAYER) {
		pkChar->SetObserverMode(true, true);
	}
#endif

	DWORD pid = pkChar->GetPlayerID();

	m_map_char.insert(std::make_pair(pid, pid));
	m_map_attender.insert(std::make_pair(pid, pid));

	return true;
}

bool COXEventManager::EnterAudience(LPCHARACTER pkChar)
{
	DWORD pid = pkChar->GetPlayerID();

	m_map_char.insert(std::make_pair(pid, pid));

	return true;
}

bool COXEventManager::AddQuiz(unsigned char level, const char* pszQuestion, bool answer)
{
	if (m_vec_quiz.size() < (size_t) level + 1)
		m_vec_quiz.resize(level + 1);

	struct tag_Quiz tmpQuiz;

	tmpQuiz.level = level;
	strlcpy(tmpQuiz.Quiz, pszQuestion, sizeof(tmpQuiz.Quiz));
	tmpQuiz.answer = answer;

	m_vec_quiz[level].push_back(tmpQuiz);
	return true;
}

bool COXEventManager::ShowQuizList(LPCHARACTER pkChar)
{
#if defined(ENABLE_TEXTS_RENEWAL)
	int c = 0;

	for (size_t i = 0; i < m_vec_quiz.size(); ++i) {
		for (size_t j = 0; j < m_vec_quiz[i].size(); ++j, ++c) {
			if (m_vec_quiz[i][j].answer) {
				pkChar->ChatPacketNew(CHAT_TYPE_INFO, 608, "%s", m_vec_quiz[i][j].Quiz);
			} else {
				pkChar->ChatPacketNew(CHAT_TYPE_INFO, 609, "%s", m_vec_quiz[i][j].Quiz);
			}
		}
	}

	pkChar->ChatPacketNew(CHAT_TYPE_INFO, 610, "%d", c);
#endif
	return true;
}

void COXEventManager::ClearQuiz()
{
	for (unsigned int i = 0; i < m_vec_quiz.size(); ++i)
	{
		m_vec_quiz[i].clear();
	}

	m_vec_quiz.clear();
}

EVENTINFO(OXEventInfoData)
{
	bool answer;

	OXEventInfoData()
	: answer( false )
	{
	}
};

EVENTFUNC(oxevent_timer)
{
	static BYTE flag = 0;
	OXEventInfoData* info = dynamic_cast<OXEventInfoData*>(event->info);

	if ( info == NULL )
	{
		sys_err( "oxevent_timer> <Factor> Null pointer" );
		return 0;
	}

	switch (flag)
	{
		case 0:
#if defined(ENABLE_TEXTS_RENEWAL)
			SendNoticeNew(CHAT_TYPE_BIG_NOTICE, 0, OXEVENT_MAP_INDEX, 579, "");
#endif
			flag++;
			return PASSES_PER_SEC(10);

		case 1:
			if (info->answer == true)
			{
				COXEventManager::instance().CheckAnswer(true);
#if defined(ENABLE_TEXTS_RENEWAL)
				SendNoticeNew(CHAT_TYPE_BIG_NOTICE, 0, OXEVENT_MAP_INDEX, 580, "");
#endif
			}
			else
			{
				COXEventManager::instance().CheckAnswer(false);
#if defined(ENABLE_TEXTS_RENEWAL)
				SendNoticeNew(CHAT_TYPE_BIG_NOTICE, 0, OXEVENT_MAP_INDEX, 581, "");
#endif
			}

#if defined(ENABLE_TEXTS_RENEWAL)
			SendNoticeNew(CHAT_TYPE_BIG_NOTICE, 0, OXEVENT_MAP_INDEX, 582, "");
#endif
			flag++;
			return PASSES_PER_SEC(5);
		case 2:
			COXEventManager::instance().WarpToAudience();
			COXEventManager::instance().SetStatus(OXEVENT_CLOSE);
#if defined(ENABLE_TEXTS_RENEWAL)
			SendNoticeNew(CHAT_TYPE_BIG_NOTICE, 0, OXEVENT_MAP_INDEX, 583, "");
#endif
			flag = 0;
			break;
	}
	return 0;
}

bool COXEventManager::Quiz(unsigned char level, int timelimit)
{
	if (m_vec_quiz.size() == 0) return false;
	if (level > m_vec_quiz.size()) level = m_vec_quiz.size() - 1;
	if (m_vec_quiz[level].size() <= 0) return false;

	if (timelimit < 0) timelimit = 30;

	int idx = number(0, m_vec_quiz[level].size()-1);

#if defined(ENABLE_TEXTS_RENEWAL)
	SendNoticeNew(CHAT_TYPE_BIG_NOTICE, 0, OXEVENT_MAP_INDEX, 584, "");
	SendNoticeNew(CHAT_TYPE_BIG_NOTICE, 0, OXEVENT_MAP_INDEX, std::stoi(m_vec_quiz[level][idx].Quiz), "");
	SendNoticeNew(CHAT_TYPE_BIG_NOTICE, 0, OXEVENT_MAP_INDEX, 585, "");
#endif
	if (m_timedEvent != NULL) {
		event_cancel(&m_timedEvent);
	}

	OXEventInfoData* answer = AllocEventInfo<OXEventInfoData>();

	answer->answer = m_vec_quiz[level][idx].answer;

	timelimit -= 15;
	m_timedEvent = event_create(oxevent_timer, answer, PASSES_PER_SEC(timelimit));

	SetStatus(OXEVENT_QUIZ);

	m_vec_quiz[level].erase(m_vec_quiz[level].begin()+idx);
	return true;
}

bool COXEventManager::CheckAnswer(bool answer)
{
	if (m_map_attender.size() <= 0) return true;

	auto iter = m_map_attender.begin();
	m_map_miss.clear();

	std::map<DWORD, DWORD>::iterator iter_tmp;

	int rect[4]{};
	if (answer != true)
	{
		rect[0] = 892600;
		rect[1] = 22900;
		rect[2] = 896300;
		rect[3] = 26400;
	}
	else
	{
		rect[0] = 896600;
		rect[1] = 22900;
		rect[2] = 900300;
		rect[3] = 26400;
	}

	LPCHARACTER pkChar = NULL;
	PIXEL_POSITION pos;
	for (; iter != m_map_attender.end();)
	{
		pkChar = CHARACTER_MANAGER::instance().FindByPID(iter->second);
		if (pkChar != NULL)
		{
			pos = pkChar->GetXYZ();

			if (pos.x < rect[0] || pos.x > rect[2] || pos.y < rect[1] || pos.y > rect[3])
			{
				pkChar->EffectPacket(SE_FAIL);
				iter_tmp = iter;
				iter++;
				m_map_attender.erase(iter_tmp);
				m_map_miss.insert(std::make_pair(pkChar->GetPlayerID(), pkChar->GetPlayerID()));
			}
			else
			{
				// pkChar->CreateFly(number(FLY_FIREWORK1, FLY_FIREWORK6), pkChar);
				char chatbuf[256];
				int len = snprintf(chatbuf, sizeof(chatbuf), "%s %u %u", number(0, 1) == 1 ? "cheer1" : "cheer2", (DWORD)pkChar->GetVID(), 0);

				// 리턴값이 sizeof(chatbuf) 이상일 경우 truncate되었다는 뜻..
				if (len < 0 || len >= (int) sizeof(chatbuf))
					len = sizeof(chatbuf) - 1;

				// \0 문자 포함
				++len;

				TPacketGCChat pack_chat;
				pack_chat.header = HEADER_GC_CHAT;
				pack_chat.size = sizeof(TPacketGCChat) + len;
				pack_chat.type = CHAT_TYPE_COMMAND;
				pack_chat.id = 0;

				TEMP_BUFFER buf;
				buf.write(&pack_chat, sizeof(TPacketGCChat));
				buf.write(chatbuf, len);

				pkChar->PacketAround(buf.read_peek(), buf.size());
				pkChar->EffectPacket(SE_SUCCESS);

				++iter;
			}
		}
		else
		{
			auto err = m_map_char.find(iter->first);
			if (err != m_map_char.end()) m_map_char.erase(err);

			auto err2 = m_map_miss.find(iter->first);
			if (err2 != m_map_miss.end()) m_map_miss.erase(err2);

			iter_tmp = iter;
			++iter;
			m_map_attender.erase(iter_tmp);
		}
	}
	return true;
}

void COXEventManager::WarpToAudience()
{
	if (m_map_miss.size() <= 0) return;

	auto iter = m_map_miss.begin();
	LPCHARACTER pkChar = NULL;

	for (; iter != m_map_miss.end(); ++iter)
	{
		pkChar = CHARACTER_MANAGER::instance().FindByPID(iter->second);

		if (pkChar && pkChar->IsPC()) {
#if defined(ENABLE_RENEWAL_OX)
			uint8_t empire = pkChar->GetEmpire();
			if (empire == 0) {
				pkChar->Show(OXEVENT_MAP_INDEX, 896300, 28900);
			} else {
				pkChar->WarpSet(g_start_position[empire][0], g_start_position[empire][1]);
			}
#else
			switch ( number(0, 3))
			{
				case 0 : pkChar->Show(OXEVENT_MAP_INDEX, 896300, 28900); break;
				case 1 : pkChar->Show(OXEVENT_MAP_INDEX, 890900, 28100); break;
				case 2 : pkChar->Show(OXEVENT_MAP_INDEX, 896600, 20500); break;
				case 3 : pkChar->Show(OXEVENT_MAP_INDEX, 902500, 28100); break;
				default : pkChar->Show(OXEVENT_MAP_INDEX, 896300, 28900); break;
			}
#endif
		}
	}

	m_map_miss.clear();
}

bool COXEventManager::CloseEvent()
{
	if (m_timedEvent != NULL) {
		event_cancel(&m_timedEvent);
	}

	auto iter = m_map_char.begin();

	LPCHARACTER pkChar = NULL;
	for (; iter != m_map_char.end(); ++iter)
	{
		pkChar = CHARACTER_MANAGER::instance().FindByPID(iter->second);

		if (pkChar != NULL)
			pkChar->WarpSet(EMPIRE_START_X(pkChar->GetEmpire()), EMPIRE_START_Y(pkChar->GetEmpire()));
	}

	m_map_char.clear();

	return true;
}

bool COXEventManager::LogWinner()
{
	auto iter = m_map_attender.begin();

	for (; iter != m_map_attender.end(); ++iter)
	{
		LPCHARACTER pkChar = CHARACTER_MANAGER::instance().FindByPID(iter->second);

		if (pkChar)
			LogManager::instance().CharLog(pkChar, 0, "OXEVENT", "LastManStanding");
	}

	return true;
}

#if defined(ENABLE_RENEWAL_OX)
bool COXEventManager::CheckIpAddress(LPCHARACTER pkChar) {
	const LPDESC d = pkChar ? pkChar->GetDesc() : NULL;
	if (!d) {
		return false;
	}

	auto it = m_map_attender.begin(), itE = m_map_attender.end();

	for (; it != itE; ++it) {
		LPCHARACTER tch = CHARACTER_MANAGER::Instance().FindByPID(it->second);
		if (!tch || !tch->GetDesc()) {
			continue;
		}

////		if (!strcmp(d->GetHostName(), tch->GetDesc()->GetHostName()) && pkChar->GetMapIndex() == tch->GetMapIndex()) {
////			pkChar->GoHome();
////			return false;
////		}
	}

	return true;
}

void COXEventManager::RemoveFromAttenderList(uint32_t pid) {
	m_map_attender.erase(pid);
}
#endif

bool COXEventManager::GiveItemToAttender(DWORD dwItemVnum, WORD count)
{
	auto iter = m_map_attender.begin();

	for (; iter != m_map_attender.end(); ++iter)
	{
		LPCHARACTER pkChar = CHARACTER_MANAGER::instance().FindByPID(iter->second);
		if (pkChar && pkChar->IsPC())
		{
#ifdef ENABLE_BLOCK_MULTIFARM
			if (pkChar->FindAffect(AFFECT_DROP_UNBLOCK, APPLY_NONE)) {
				pkChar->AutoGiveItem(dwItemVnum, count);
				LogManager::instance().ItemLog(pkChar->GetPlayerID(), 0, count, dwItemVnum, "OXEVENT_REWARD", "", pkChar->GetDesc()->GetHostName(), dwItemVnum);
			}
#else
			pkChar->AutoGiveItem(dwItemVnum, count);
			LogManager::instance().ItemLog(pkChar->GetPlayerID(), 0, count, dwItemVnum, "OXEVENT_REWARD", "", pkChar->GetDesc()->GetHostName(), dwItemVnum);
#endif
		}
	}

	return true;
}

