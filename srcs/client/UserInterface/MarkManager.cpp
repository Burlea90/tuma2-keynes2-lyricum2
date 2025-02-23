#include "stdafx.h"
#include "MarkManager.h"

#if _MSC_VER < 1200
#include "crc32.h"
#endif

CGuildMarkImage * CGuildMarkManager::__NewImage()
{
	return new CGuildMarkImage;
}

void CGuildMarkManager::__DeleteImage(CGuildMarkImage * pkImgDel)
{
	delete pkImgDel;
}

CGuildMarkManager::CGuildMarkManager()
{
#if _MSC_VER >= 1200
	_mkdir("mark");
#endif
	// 남은 mark id 셋을 만든다. (서버용)
	for (DWORD i = 0; i < MAX_IMAGE_COUNT * CGuildMarkImage::MARK_TOTAL_COUNT; ++i)
		m_setFreeMarkID.insert(i);
}

CGuildMarkManager::~CGuildMarkManager()
{
	for (std::map<DWORD, CGuildMarkImage *>::iterator it = m_mapIdx_Image.begin(); it != m_mapIdx_Image.end(); ++it)
		__DeleteImage(it->second);

	m_mapIdx_Image.clear();
}

bool CGuildMarkManager::GetMarkImageFilename(DWORD imgIdx, std::string & path) const
{
	if (imgIdx >= MAX_IMAGE_COUNT)
		return false;

	char buf[64];
	snprintf(buf, sizeof(buf), "mark/%s_%lu.tga", m_pathPrefix.c_str(), imgIdx);
	path = buf;
	return true;
}

void CGuildMarkManager::SetMarkPathPrefix(const char * prefix)
{
	m_pathPrefix = prefix;
}

// 마크 인덱스 불러오기 (서버에서만 사용)
bool CGuildMarkManager::LoadMarkIndex()
{
	char buf[64];
	snprintf(buf, sizeof(buf), "mark/%s_index", m_pathPrefix.c_str());
	FILE * fp = fopen(buf, "r");

	if (!fp)
		return false;

	DWORD guildID;
	DWORD markID;

	char line[256];

	while (fgets(line, sizeof(line)-1, fp))
	{
		sscanf(line, "%lu %lu", &guildID, &markID);
		line[0] = '\0';
		AddMarkIDByGuildID(guildID, markID);
	}

	LoadMarkImages();

	fclose(fp);
	return true;
}

bool CGuildMarkManager::SaveMarkIndex()
{
	char buf[64];
	snprintf(buf, sizeof(buf), "mark/%s_index", m_pathPrefix.c_str());
	FILE * fp = fopen(buf, "w");

	if (!fp)
	{
		TraceError("MarkManager::SaveMarkData: cannot open index file.");
		return false;
	}

	for (std::map<DWORD, DWORD>::iterator it = m_mapGID_MarkID.begin(); it != m_mapGID_MarkID.end(); ++it)
		fprintf(fp, "%d %d\n", it->first, it->second);

	fclose(fp);
#if !defined(DISTRIBUTE)
	Tracef("MarkManager::SaveMarkData: index count %d", m_mapGID_MarkID.size());
#endif
	return true;
}

void CGuildMarkManager::LoadMarkImages()
{
	bool isMarkExists[MAX_IMAGE_COUNT];
	memset(isMarkExists, 0, sizeof(isMarkExists));

	for (std::map<DWORD, DWORD>::iterator it = m_mapGID_MarkID.begin(); it != m_mapGID_MarkID.end(); ++it)
	{
		DWORD markID = it->second;

		if (markID < MAX_IMAGE_COUNT * CGuildMarkImage::MARK_TOTAL_COUNT)
			isMarkExists[markID / CGuildMarkImage::MARK_TOTAL_COUNT] = true;
	}

	for (DWORD i = 0; i < MAX_IMAGE_COUNT; ++i)
		if (isMarkExists[i])
			__GetImage(i);
}

void CGuildMarkManager::SaveMarkImage(DWORD imgIdx)
{
	std::string path;

	if (GetMarkImageFilename(imgIdx, path))
		if (!__GetImage(imgIdx)->Save(path.c_str()))
			TraceError("%s Save failed\n", path.c_str());
}

CGuildMarkImage * CGuildMarkManager::__GetImage(DWORD imgIdx)
{
	std::map<DWORD, CGuildMarkImage *>::iterator it = m_mapIdx_Image.find(imgIdx);

	if (it == m_mapIdx_Image.end())
	{
		std::string imagePath;

		if (GetMarkImageFilename(imgIdx, imagePath))
		{
			CGuildMarkImage * pkImage = __NewImage();
			m_mapIdx_Image.insert(std::map<DWORD, CGuildMarkImage *>::value_type(imgIdx, pkImage));
			pkImage->Load(imagePath.c_str());
			return pkImage;
		}
		else
			return NULL;
	}
	else
		return it->second;
}

bool CGuildMarkManager::AddMarkIDByGuildID(DWORD guildID, DWORD markID)
{
	if (markID >= MAX_IMAGE_COUNT * CGuildMarkImage::MARK_TOTAL_COUNT)
		return false;

	m_mapGID_MarkID.insert(std::map<DWORD, DWORD>::value_type(guildID, markID));
	m_setFreeMarkID.erase(markID);
	return true;
}

DWORD CGuildMarkManager::GetMarkID(DWORD guildID)
{
	std::map<DWORD, DWORD>::iterator it = m_mapGID_MarkID.find(guildID);

	if (it == m_mapGID_MarkID.end())
		return INVALID_MARK_ID;

	return it->second;
}

DWORD CGuildMarkManager::__AllocMarkID(DWORD guildID)
{
	std::set<DWORD>::iterator it = m_setFreeMarkID.lower_bound(0);

	if (it == m_setFreeMarkID.end())
		return INVALID_MARK_ID;

	DWORD markID = *it;

	DWORD imgIdx = markID / CGuildMarkImage::MARK_TOTAL_COUNT;
	CGuildMarkImage * pkImage = __GetImage(imgIdx); // 이미지가 없다면 만들기 위해

	if (pkImage && AddMarkIDByGuildID(guildID, markID))
		return markID;

	return INVALID_MARK_ID;
}

DWORD CGuildMarkManager::GetMarkImageCount() const
{
	return m_mapIdx_Image.size();
}

DWORD CGuildMarkManager::GetMarkCount() const
{
	return m_mapGID_MarkID.size();
}

// SERVER
void CGuildMarkManager::CopyMarkIdx(char * pcBuf) const
{
	WORD * pwBuf = (WORD *) pcBuf;

	for (std::map<DWORD, DWORD>::const_iterator it = m_mapGID_MarkID.begin(); it != m_mapGID_MarkID.end(); ++it)
	{
		*(pwBuf++) = it->first; // guild id
		*(pwBuf++) = it->second; // mark id
	}
}

// SERVER
DWORD CGuildMarkManager::SaveMark(DWORD guildID, BYTE * pbMarkImage)
{
	DWORD idMark;

	if ((idMark = GetMarkID(guildID)) == INVALID_MARK_ID)
	{
		if ((idMark = __AllocMarkID(guildID)) == INVALID_MARK_ID)
		{
			TraceError("CGuildMarkManager: cannot alloc mark id %u", guildID);
			return false;
		}
		else
		{
#if !defined(DISTRIBUTE)
			Tracef("SaveMark: mark id alloc %u", idMark);
#endif
		}
	}
	else
	{
#if !defined(DISTRIBUTE)
		Tracef("SaveMark: mark id found %u", idMark);
#endif
	}

	DWORD imgIdx = (idMark / CGuildMarkImage::MARK_TOTAL_COUNT);
	CGuildMarkImage * pkImage = __GetImage(imgIdx);

	if (pkImage)
	{
#if !defined(DISTRIBUTE)
		Tracef("imgidx %u ", imgIdx);
#endif

		std::string pathImage;
		GetMarkImageFilename(imgIdx, pathImage);
		pkImage->Save(pathImage.c_str());

		SaveMarkIndex();
	}

	return idMark;
}

// SERVER
void CGuildMarkManager::DeleteMark(DWORD guildID)
{
	std::map<DWORD, DWORD>::iterator it = m_mapGID_MarkID.find(guildID);

	if (it == m_mapGID_MarkID.end())
		return;

	CGuildMarkImage * pkImage;

	if ((pkImage = __GetImage(it->second / CGuildMarkImage::MARK_TOTAL_COUNT)) != NULL)
		pkImage->DeleteMark(it->second % CGuildMarkImage::MARK_TOTAL_COUNT);

	m_mapGID_MarkID.erase(it);
	m_setFreeMarkID.insert(it->second);

	SaveMarkIndex();
}

// SERVER
void CGuildMarkManager::GetDiffBlocks(DWORD imgIdx, const DWORD * crcList, std::map<BYTE, const SGuildMarkBlock *> & mapDiffBlocks)
{
	mapDiffBlocks.clear();

	// 클라이언트에서 서버에 없는 이미지를 요청할 수는 없다.
	if (m_mapIdx_Image.end() == m_mapIdx_Image.find(imgIdx))
	{
//		TraceError("invalid idx %u", imgIdx); // @warme668
		return;
	}

	CGuildMarkImage * p = __GetImage(imgIdx);

	if (p)
		p->GetDiffBlocks(crcList, mapDiffBlocks);
}

// CLIENT
bool CGuildMarkManager::SaveBlockFromCompressedData(DWORD imgIdx, DWORD posBlock, const BYTE * pbBlock, DWORD dwSize)
{
	CGuildMarkImage * pkImage = __GetImage(imgIdx);

	if (pkImage)
		pkImage->SaveBlockFromCompressedData(posBlock, pbBlock, dwSize);

	return false;
}

// CLIENT
bool CGuildMarkManager::GetBlockCRCList(DWORD imgIdx, DWORD * crcList)
{
	// 클라이언트에서 서버에 없는 이미지를 요청할 수는 없다.
	if (m_mapIdx_Image.end() == m_mapIdx_Image.find(imgIdx))
	{
//		TraceError("invalid idx %u", imgIdx); // @warme668
		return false;
	}

	CGuildMarkImage * p = __GetImage(imgIdx);

	if (p)
		p->GetBlockCRCList(crcList);

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////
// Symbol
///////////////////////////////////////////////////////////////////////////////////////
const CGuildMarkManager::TGuildSymbol * CGuildMarkManager::GetGuildSymbol(DWORD guildID)
{
	std::map<DWORD, TGuildSymbol>::iterator it = m_mapSymbol.find(guildID);

	if (it == m_mapSymbol.end())
		return NULL;

	return &it->second;
}

bool CGuildMarkManager::LoadSymbol(const char* filename)
{
	FILE* fp = fopen(filename, "rb");

	if (!fp)
		return true;
	else
	{
		DWORD symbolCount;
		fread(&symbolCount, 4, 1, fp);

		for(DWORD i = 0; i < symbolCount; i++)
		{
			DWORD guildID;
			DWORD dwSize;
			fread(&guildID, 4, 1, fp);
			fread(&dwSize, 4, 1, fp);

			TGuildSymbol gs;
			gs.raw.resize(dwSize);
			fread(&gs.raw[0], 1, dwSize, fp);
			gs.crc = GetCRC32(reinterpret_cast<const char*>(&gs.raw[0]), dwSize);
			m_mapSymbol.insert(std::make_pair(guildID, gs));
		}
	}

	fclose(fp);
	return true;
}

void CGuildMarkManager::SaveSymbol(const char* filename)
{
	FILE* fp = fopen(filename, "wb");
	if (!fp)
	{
		TraceError("Cannot open Symbol file (name: %s)", filename);
		return;
	}

	DWORD symbolCount = m_mapSymbol.size();
	fwrite(&symbolCount, 4, 1, fp);

	for (std::map<DWORD, TGuildSymbol>::iterator it = m_mapSymbol.begin(); it != m_mapSymbol.end(); ++it)
	{
		DWORD guildID = it->first;
		DWORD dwSize = it->second.raw.size();
		fwrite(&guildID, 4, 1, fp);
		fwrite(&dwSize, 4, 1, fp);
		fwrite(&it->second.raw[0], 1, dwSize, fp);
	}

	fclose(fp);
}

void CGuildMarkManager::UploadSymbol(DWORD guildID, int iSize, const BYTE* pbyData)
{
#if !defined(DISTRIBUTE)
	Tracef("GuildSymbolUpload guildID %lu Size %d", guildID, iSize);
#endif

	if (m_mapSymbol.find(guildID) == m_mapSymbol.end())
		m_mapSymbol.insert(std::make_pair(guildID, TGuildSymbol()));

	TGuildSymbol& rSymbol = m_mapSymbol[guildID];
	rSymbol.raw.clear();

	if (iSize > 0)
	{
		rSymbol.raw.reserve(iSize);
		std::copy(pbyData, (pbyData + iSize), std::back_inserter(rSymbol.raw));
		rSymbol.crc = GetCRC32(reinterpret_cast<const char*>(pbyData), iSize);
	}
}

#ifdef __UNITTEST__
#include "lzo_manager.h"

Guild::CGuildMarkManager * mgr;

void heartbeat(LPHEART ht, int pulse)
{
	return;
}

void SaveMark(DWORD guildID, const char * filename)
{
	ILuint m_uImg;

	ilGenImages(1, &m_uImg);
	ilBindImage(m_uImg);
	ilEnable(IL_ORIGIN_SET);
	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);

	if (ilLoad(IL_TYPE_UNKNOWN, (const ILstring) filename))
	{
		ILuint width = ilGetInteger(IL_IMAGE_WIDTH);
		ILuint height = ilGetInteger(IL_IMAGE_HEIGHT);

		ilConvertImage(IL_BGRA, IL_UNSIGNED_BYTE);

		BYTE * data = (BYTE *) malloc(sizeof(DWORD) * width * height);
		ilCopyPixels(0, 0, 0, width, height, 1, IL_BGRA, IL_UNSIGNED_BYTE, data);
		ilDeleteImages(1, &m_uImg);

#if !defined(DISTRIBUTE)
		Tracef("%s w%u h%u ", filename, width, height);
#endif
		mgr->SaveMark(guildID, data);
	}
	else
	{
#if !defined(DISTRIBUTE)
		Tracef("%s cannot open file.\n", filename);
#endif
	}
}

int main(int argc, char **argv)
{
	LZOManager lzo;
	char f[64];

	srandom(time(0));

	ilInit(); // DevIL Initialize
	thecore_init(25, heartbeat);

	std::vector<int> vec_guild_id;

	for (int i = 1; i < 1281; ++i)
		vec_guild_id.push_back(i);

	mgr = new CGuildMarkManager;
	mgr->LoadMarkData(vec_guild_id);
	/*
	for (int i = 1401; i < 1500; ++i)
	{
		snprintf(f, sizeof(f), "%lu.jpg", (random() % 5) + 1);
		//SaveMark(i, f);
		mgr->DeleteMark(i);
	}
	*/
	//snprintf(f, sizeof(f), "%lu.jpg", (random() % 5) + 1);
	//SaveMark(1, f);
	DWORD idx_client[CGuildMarkImage::BLOCK_TOTAL_COUNT];
	DWORD idx_server[CGuildMarkImage::BLOCK_TOTAL_COUNT];

	mgr->GetBlockCRCList(0, idx_client);
	mgr->GetBlockCRCList(1, idx_server);

	std::map<BYTE, const SGuildMarkBlock *> mapDiff;
	mgr->GetDiffBlocks(1, idx_client, mapDiff);

#if !defined(DISTRIBUTE)
	Tracef("#1 Diff %u\n", mapDiff.size());
#endif

	for (itertype(mapDiff) it = mapDiff.begin(); it != mapDiff.end(); ++it)
	{
#if !defined(DISTRIBUTE)
		Tracef("Put Block pos %u crc %u\n", it->first, it->second->m_crc);
#endif

		mgr->SaveBlockFromCompressedData(0, it->first, it->second->m_abCompBuf, it->second->m_sizeCompBuf);
	}

	mgr->GetBlockCRCList(0, idx_client);
	mgr->GetDiffBlocks(1, idx_client, mapDiff);
#if !defined(DISTRIBUTE)
	Tracef("#2 Diff %u\n", mapDiff.size());
#endif
	delete mgr;
	return 1;
}
#endif
