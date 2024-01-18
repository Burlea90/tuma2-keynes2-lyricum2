#ifndef __INC_REFINE_H
#define __INC_REFINE_H

#include "constants.h"

#define REFINE_INCREASE "REFINE.INCREASE_PERCENTAGE"

enum
{
	BLACKSMITH_MOB = 20016,
	ALCHEMIST_MOB = 20001,
	BLACKSMITH_WEAPON_MOB = 20044,
	BLACKSMITH_ARMOR_MOB = 20045,
	BLACKSMITH_ACCESSORY_MOB = 20046,
	DEVILTOWER_BLACKSMITH_WEAPON_MOB = 20074,
	DEVILTOWER_BLACKSMITH_ARMOR_MOB = 20075,
	DEVILTOWER_BLACKSMITH_ACCESSORY_MOB = 20076,
	BLACKSMITH2_MOB	= 20091,
};

#ifdef ENABLE_FEATURES_REFINE_SYSTEM
enum
{
	REFINE_PERCENTAGE_LOW = 5,
	REFINE_PERCENTAGE_MEDIUM = 10,
	REFINE_PERCENTAGE_EXTRA = 15,
	REFINE_VNUM_POTION_LOW = 56001,
	REFINE_VNUM_POTION_MEDIUM = 56002,
	REFINE_VNUM_POTION_EXTRA = 56003,
};
#endif

class CRefineManager : public singleton<CRefineManager>
{
	typedef std::map<uint32_t, TRefineTable> TRefineRecipeMap;

	public:
		CRefineManager();
		virtual ~CRefineManager();

		bool Initialize(TRefineTable*, int32_t);
		const TRefineTable* GetRefineRecipe(uint32_t);

#ifdef ENABLE_FEATURES_REFINE_SYSTEM
		bool GetPercentage(LPCHARACTER, uint8_t, uint8_t, uint8_t, uint8_t, LPITEM);
		void Increase(LPCHARACTER, uint8_t, uint8_t, uint8_t);
		void Reset(LPCHARACTER);
		int Result(LPCHARACTER);
#endif

	private:
		TRefineRecipeMap m_map_RefineRecipe;

};
#endif
