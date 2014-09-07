// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "StarSystem.h"
#include "Sector.h"
#include "Galaxy.h"
#include "GalaxyCache.h"
#include "GalaxyGenerator.h"
#include "StarSystemGenerator.h"
#include "Factions.h"

#include "Serializer.h"
#include "Pi.h"
#include "LuaNameGen.h"
#include "enum_table.h"
#include <map>
#include <string>
#include <algorithm>
#include "utils.h"
#include "Orbit.h"
#include "Lang.h"
#include "StringF.h"
#include <SDL_stdinc.h>
#include "EnumStrings.h"

static const double CELSIUS	= 273.15;
//#define DEBUG_DUMP

// minimum moon mass a little under Europa's
static const fixed MIN_MOON_MASS = fixed(1,30000); // earth masses
static const fixed MIN_MOON_DIST = fixed(15,10000); // AUs
static const fixed MAX_MOON_DIST = fixed(2, 100); // AUs
static const fixed PLANET_MIN_SEPARATION = fixed(135,100);

// very crudely
static const fixed AU_EARTH_RADIUS = fixed(3, 65536);

static const fixed SUN_MASS_TO_EARTH_MASS = fixed(332998,1);

static const fixed FIXED_PI = fixed(103993,33102);

// indexed by enum type turd
const Uint8 StarSystem::starColors[][3] = {
	{ 0, 0, 0 }, // gravpoint
	{ 128, 0, 0 }, // brown dwarf
	{ 102, 102, 204 }, // white dwarf
	{ 255, 51, 0 }, // M
	{ 255, 153, 26 }, // K
	{ 255, 255, 102 }, // G
	{ 255, 255, 204 }, // F
	{ 255, 255, 255 }, // A
	{ 178, 178, 255 }, // B
	{ 255, 178, 255 }, // O
	{ 255, 51, 0 }, // M Giant
	{ 255, 153, 26 }, // K Giant
	{ 255, 255, 102 }, // G Giant
	{ 255, 255, 204 }, // F Giant
	{ 255, 255, 255 }, // A Giant
	{ 178, 178, 255 }, // B Giant
	{ 255, 178, 255 }, // O Giant
	{ 255, 51, 0 }, // M Super Giant
	{ 255, 153, 26 }, // K Super Giant
	{ 255, 255, 102 }, // G Super Giant
	{ 255, 255, 204 }, // F Super Giant
	{ 255, 255, 255 }, // A Super Giant
	{ 178, 178, 255 }, // B Super Giant
	{ 255, 178, 255 }, // O Super Giant
	{ 255, 51, 0 }, // M Hyper Giant
	{ 255, 153, 26 }, // K Hyper Giant
	{ 255, 255, 102 }, // G Hyper Giant
	{ 255, 255, 204 }, // F Hyper Giant
	{ 255, 255, 255 }, // A Hyper Giant
	{ 178, 178, 255 }, // B Hyper Giant
	{ 255, 178, 255 }, // O Hyper Giant
	{ 255, 51, 0 }, // Red/M Wolf Rayet Star
	{ 178, 178, 255 }, // Blue/B Wolf Rayet Star
	{ 255, 178, 255 }, // Purple-Blue/O Wolf Rayet Star
	{ 76, 178, 76 }, // Stellar Blackhole
	{ 51, 230, 51 }, // Intermediate mass Black-hole
	{ 0, 255, 0 }, // Super massive black hole
};

// indexed by enum type turd
const Uint8 StarSystem::starRealColors[][3] = {
	{ 0, 0, 0 }, // gravpoint
	{ 128, 0, 0 }, // brown dwarf
	{ 255, 255, 255 }, // white dwarf
	{ 255, 128, 51 }, // M
	{ 255, 255, 102 }, // K
	{ 255, 255, 242 }, // G
	{ 255, 255, 255 }, // F
	{ 255, 255, 255 }, // A
	{ 204, 204, 255 }, // B
	{ 255, 204, 255 },  // O
	{ 255, 128, 51 }, // M Giant
	{ 255, 255, 102 }, // K Giant
	{ 255, 255, 242 }, // G Giant
	{ 255, 255, 255 }, // F Giant
	{ 255, 255, 255 }, // A Giant
	{ 204, 204, 255 }, // B Giant
	{ 255, 204, 255 },  // O Giant
	{ 255, 128, 51 }, // M Super Giant
	{ 255, 255, 102 }, // K Super Giant
	{ 255, 255, 242 }, // G Super Giant
	{ 255, 255, 255 }, // F Super Giant
	{ 255, 255, 255 }, // A Super Giant
	{ 204, 204, 255 }, // B Super Giant
	{ 255, 204, 255 },  // O Super Giant
	{ 255, 128, 51 }, // M Hyper Giant
	{ 255, 255, 102 }, // K Hyper Giant
	{ 255, 255, 242 }, // G Hyper Giant
	{ 255, 255, 255 }, // F Hyper Giant
	{ 255, 255, 255 }, // A Hyper Giant
	{ 204, 204, 255 }, // B Hyper Giant
	{ 255, 204, 255 },  // O Hyper Giant
	{ 255, 153, 153 }, // M WF
	{ 204, 204, 255 }, // B WF
	{ 255, 204, 255 },  // O WF
	{ 255, 255, 255 },  // small Black hole
	{ 16, 0, 20 }, // med BH
	{ 10, 0, 16 }, // massive BH
};

const double StarSystem::starLuminosities[] = {
	0,
	0.0003, // brown dwarf
	0.1, // white dwarf
	0.08, // M0
	0.38, // K0
	1.2, // G0
	5.1, // F0
	24.0, // A0
	100.0, // B0
	200.0, // O5
	1000.0, // M0 Giant
	2000.0, // K0 Giant
	4000.0, // G0 Giant
	6000.0, // F0 Giant
	8000.0, // A0 Giant
	9000.0, // B0 Giant
	12000.0, // O5 Giant
	12000.0, // M0 Super Giant
	14000.0, // K0 Super Giant
	18000.0, // G0 Super Giant
	24000.0, // F0 Super Giant
	30000.0, // A0 Super Giant
	50000.0, // B0 Super Giant
	100000.0, // O5 Super Giant
	125000.0, // M0 Hyper Giant
	150000.0, // K0 Hyper Giant
	175000.0, // G0 Hyper Giant
	200000.0, // F0 Hyper Giant
	200000.0, // A0 Hyper Giant
	200000.0, // B0 Hyper Giant
	200000.0, // O5 Hyper Giant
	50000.0, // M WF
	100000.0, // B WF
	200000.0, // O WF
	0.0003, // Stellar Black hole
	0.00003, // IM Black hole
	0.000003, // Supermassive Black hole
};

const float StarSystem::starScale[] = {  // Used in sector view
	0,
	0.6f, // brown dwarf
	0.5f, // white dwarf
	0.7f, // M
	0.8f, // K
	0.8f, // G
	0.9f, // F
	1.0f, // A
	1.1f, // B
	1.1f, // O
	1.3f, // M Giant
	1.2f, // K G
	1.2f, // G G
	1.2f, // F G
	1.1f, // A G
	1.1f, // B G
	1.2f, // O G
	1.8f, // M Super Giant
	1.6f, // K SG
	1.5f, // G SG
	1.5f, // F SG
	1.4f, // A SG
	1.3f, // B SG
	1.3f, // O SG
	2.5f, // M Hyper Giant
	2.2f, // K HG
	2.2f, // G HG
	2.1f, // F HG
	2.1f, // A HG
	2.0f, // B HG
	1.9f, // O HG
	1.1f, // M WF
	1.3f, // B WF
	1.6f, // O WF
	1.0f, // Black hole
	2.5f, // Intermediate-mass blackhole
	4.0f  // Supermassive blackhole
};

const fixed StarSystemLegacyGeneratorBase::starMetallicities[] = {
	fixed(1,1), // GRAVPOINT - for planets that orbit them
	fixed(9,10), // brown dwarf
	fixed(5,10), // white dwarf
	fixed(7,10), // M0
	fixed(6,10), // K0
	fixed(5,10), // G0
	fixed(4,10), // F0
	fixed(3,10), // A0
	fixed(2,10), // B0
	fixed(1,10), // O5
	fixed(8,10), // M0 Giant
	fixed(65,100), // K0 Giant
	fixed(55,100), // G0 Giant
	fixed(4,10), // F0 Giant
	fixed(3,10), // A0 Giant
	fixed(2,10), // B0 Giant
	fixed(1,10), // O5 Giant
	fixed(9,10), // M0 Super Giant
	fixed(7,10), // K0 Super Giant
	fixed(6,10), // G0 Super Giant
	fixed(4,10), // F0 Super Giant
	fixed(3,10), // A0 Super Giant
	fixed(2,10), // B0 Super Giant
	fixed(1,10), // O5 Super Giant
	fixed(1,1), // M0 Hyper Giant
	fixed(7,10), // K0 Hyper Giant
	fixed(6,10), // G0 Hyper Giant
	fixed(4,10), // F0 Hyper Giant
	fixed(3,10), // A0 Hyper Giant
	fixed(2,10), // B0 Hyper Giant
	fixed(1,10), // O5 Hyper Giant
	fixed(1,1), // M WF
	fixed(8,10), // B WF
	fixed(6,10), // O WF
	fixed(1,1), //  S BH	Blackholes, give them high metallicity,
	fixed(1,1), // IM BH	so any rocks that happen to be there
	fixed(1,1)  // SM BH	may be mining hotspots. FUN :)
};

const StarSystemLegacyGeneratorBase::StarTypeInfo StarSystemLegacyGeneratorBase::starTypeInfo[] = {
	{
		SystemBody::SUPERTYPE_NONE, {}, {},
        0, 0
	}, {
		SystemBody::SUPERTYPE_STAR, //Brown Dwarf
		{2,8}, {10,30},
		1000, 2000
	}, {
		SystemBody::SUPERTYPE_STAR,  //white dwarf
		{20,100}, {1,2},
		4000, 40000
	}, {
		SystemBody::SUPERTYPE_STAR, //M
		{10,47}, {30,60},
		2000, 3500
	}, {
		SystemBody::SUPERTYPE_STAR, //K
		{50,78}, {60,100},
		3500, 5000
	}, {
		SystemBody::SUPERTYPE_STAR, //G
		{80,110}, {80,120},
		5000, 6000
	}, {
		SystemBody::SUPERTYPE_STAR, //F
		{115,170}, {110,150},
		6000, 7500
	}, {
		SystemBody::SUPERTYPE_STAR, //A
		{180,320}, {120,220},
		7500, 10000
	}, {
		SystemBody::SUPERTYPE_STAR,  //B
		{200,300}, {120,290},
		10000, 30000
	}, {
		SystemBody::SUPERTYPE_STAR, //O
		{300,400}, {200,310},
		30000, 60000
	}, {
		SystemBody::SUPERTYPE_STAR, //M Giant
		{60,357}, {2000,5000},
		2500, 3500
	}, {
		SystemBody::SUPERTYPE_STAR, //K Giant
		{125,500}, {1500,3000},
		3500, 5000
	}, {
		SystemBody::SUPERTYPE_STAR, //G Giant
		{200,800}, {1000,2000},
		5000, 6000
	}, {
		SystemBody::SUPERTYPE_STAR, //F Giant
		{250,900}, {800,1500},
		6000, 7500
	}, {
		SystemBody::SUPERTYPE_STAR, //A Giant
		{400,1000}, {600,1000},
		7500, 10000
	}, {
		SystemBody::SUPERTYPE_STAR,  //B Giant
		{500,1000}, {600,1000},
		10000, 30000
	}, {
		SystemBody::SUPERTYPE_STAR, //O Giant
		{600,1200}, {600,1000},
		30000, 60000
	}, {
		SystemBody::SUPERTYPE_STAR, //M Super Giant
		{1050,5000}, {7000,15000},
		2500, 3500
	}, {
		SystemBody::SUPERTYPE_STAR, //K Super Giant
		{1100,5000}, {5000,9000},
		3500, 5000
	}, {
		SystemBody::SUPERTYPE_STAR, //G Super Giant
		{1200,5000}, {4000,8000},
		5000, 6000
	}, {
		SystemBody::SUPERTYPE_STAR, //F Super Giant
		{1500,6000}, {3500,7000},
		6000, 7500
	}, {
		SystemBody::SUPERTYPE_STAR, //A Super Giant
		{2000,8000}, {3000,6000},
		7500, 10000
	}, {
		SystemBody::SUPERTYPE_STAR,  //B Super Giant
		{3000,9000}, {2500,5000},
		10000, 30000
	}, {
		SystemBody::SUPERTYPE_STAR, //O Super Giant
		{5000,10000}, {2000,4000},
		30000, 60000
	}, {
		SystemBody::SUPERTYPE_STAR, //M Hyper Giant
		{5000,15000}, {20000,40000},
		2500, 3500
	}, {
		SystemBody::SUPERTYPE_STAR, //K Hyper Giant
		{5000,17000}, {17000,25000},
		3500, 5000
	}, {
		SystemBody::SUPERTYPE_STAR, //G Hyper Giant
		{5000,18000}, {14000,20000},
		5000, 6000
	}, {
		SystemBody::SUPERTYPE_STAR, //F Hyper Giant
		{5000,19000}, {12000,17500},
		6000, 7500
	}, {
		SystemBody::SUPERTYPE_STAR, //A Hyper Giant
		{5000,20000}, {10000,15000},
		7500, 10000
	}, {
		SystemBody::SUPERTYPE_STAR,  //B Hyper Giant
		{5000,23000}, {6000,10000},
		10000, 30000
	}, {
		SystemBody::SUPERTYPE_STAR, //O Hyper Giant
		{10000,30000}, {4000,7000},
		30000, 60000
	}, {
		SystemBody::SUPERTYPE_STAR,  // M WF
		{2000,5000}, {2500,5000},
		25000, 35000
	}, {
		SystemBody::SUPERTYPE_STAR,  // B WF
		{2000,7500}, {2500,5000},
		35000, 45000
	}, {
		SystemBody::SUPERTYPE_STAR,  // O WF
		{2000,10000}, {2500,5000},
		45000, 60000
	}, {
		SystemBody::SUPERTYPE_STAR,  // S BH
		{20,2000}, {0,0},	// XXX black holes are < 1 Sol radii big; this is clamped to a non-zero value later
		10, 24
	}, {
		SystemBody::SUPERTYPE_STAR,  // IM BH
		{900000,1000000}, {100,500},
		1, 10
	}, {
		SystemBody::SUPERTYPE_STAR,  // SM BH
		{2000000,5000000}, {10000,20000},
		10, 24
	}
/*	}, {
		SystemBody::SUPERTYPE_GAS_GIANT,
		{}, 950, Lang::MEDIUM_GAS_GIANT,
	}, {
		SystemBody::SUPERTYPE_GAS_GIANT,
		{}, 1110, Lang::LARGE_GAS_GIANT,
	}, {
		SystemBody::SUPERTYPE_GAS_GIANT,
		{}, 1500, Lang::VERY_LARGE_GAS_GIANT,
	}, {
		SystemBody::SUPERTYPE_ROCKY_PLANET,
		{}, 1, Lang::ASTEROID,
		"icons/object_planet_asteroid.png"
	}, {
		SystemBody::SUPERTYPE_ROCKY_PLANET,
		{}, 2, "Large asteroid",
	}, {
		SystemBody::SUPERTYPE_ROCKY_PLANET,
		{}, 26, "Small, rocky dwarf planet", // moon radius
	}, {
		SystemBody::SUPERTYPE_ROCKY_PLANET,
		{}, 26, "Small, rocky dwarf planet", // dwarf2 for moon-like colours
	}, {
		SystemBody::SUPERTYPE_ROCKY_PLANET,
		{}, 52, "Small, rocky planet with a thin atmosphere", // mars radius
	}, {
		SystemBody::SUPERTYPE_ROCKY_PLANET,
		{}, 100, "Rocky frozen planet with a thin nitrogen atmosphere", // earth radius
	}, {
		SystemBody::SUPERTYPE_ROCKY_PLANET,
		{}, 100, "Dead world that once housed it's own intricate ecosystem.", // earth radius
	}, {
		SystemBody::SUPERTYPE_ROCKY_PLANET,
		{}, 100, "Rocky planet with a carbon dioxide atmosphere",
	}, {
		SystemBody::SUPERTYPE_ROCKY_PLANET,
		{}, 100, "Rocky planet with a methane atmosphere",
	}, {
		SystemBody::SUPERTYPE_ROCKY_PLANET,
		{}, 100, "Water world with vast oceans and a thick nitrogen atmosphere",
	}, {
		SystemBody::SUPERTYPE_ROCKY_PLANET,
		{}, 100, "Rocky planet with a thick carbon dioxide atmosphere",
	}, {
		SystemBody::SUPERTYPE_ROCKY_PLANET,
		{}, 100, "Rocky planet with a thick methane atmosphere",
	}, {
		SystemBody::SUPERTYPE_ROCKY_PLANET,
		{}, 100, "Highly volcanic world",
	}, {
		SystemBody::SUPERTYPE_ROCKY_PLANET,
		{}, 100, "World with indigenous life and an oxygen atmosphere",
	}, {
		SystemBody::SUPERTYPE_ROCKY_PLANET,
		{}, 60, "Marginal terraformed world with minimal plant life",
	}, {
		SystemBody::SUPERTYPE_ROCKY_PLANET,
		{}, 90, "Fully terraformed world with introduced species from numerous successful colonies",
	}, {
		SystemBody::SUPERTYPE_STARPORT,
		{}, 0, Lang::ORBITAL_STARPORT,
	}, {
		SystemBody::SUPERTYPE_STARPORT,
		{}, 0, Lang::STARPORT,
	}*/
};

SystemBody::BodySuperType SystemBody::GetSuperType() const
{
	PROFILE_SCOPED()
	switch (m_type) {
		case TYPE_BROWN_DWARF:
		case TYPE_WHITE_DWARF:
		case TYPE_STAR_M:
		case TYPE_STAR_K:
		case TYPE_STAR_G:
		case TYPE_STAR_F:
		case TYPE_STAR_A:
		case TYPE_STAR_B:
		case TYPE_STAR_O:
		case TYPE_STAR_M_GIANT:
		case TYPE_STAR_K_GIANT:
		case TYPE_STAR_G_GIANT:
		case TYPE_STAR_F_GIANT:
		case TYPE_STAR_A_GIANT:
		case TYPE_STAR_B_GIANT:
		case TYPE_STAR_O_GIANT:
		case TYPE_STAR_M_SUPER_GIANT:
		case TYPE_STAR_K_SUPER_GIANT:
		case TYPE_STAR_G_SUPER_GIANT:
		case TYPE_STAR_F_SUPER_GIANT:
		case TYPE_STAR_A_SUPER_GIANT:
		case TYPE_STAR_B_SUPER_GIANT:
		case TYPE_STAR_O_SUPER_GIANT:
		case TYPE_STAR_M_HYPER_GIANT:
		case TYPE_STAR_K_HYPER_GIANT:
		case TYPE_STAR_G_HYPER_GIANT:
		case TYPE_STAR_F_HYPER_GIANT:
		case TYPE_STAR_A_HYPER_GIANT:
		case TYPE_STAR_B_HYPER_GIANT:
		case TYPE_STAR_O_HYPER_GIANT:
		case TYPE_STAR_M_WF:
		case TYPE_STAR_B_WF:
		case TYPE_STAR_O_WF:
		case TYPE_STAR_S_BH:
		case TYPE_STAR_IM_BH:
		case TYPE_STAR_SM_BH:
		     return SUPERTYPE_STAR;
		case TYPE_PLANET_GAS_GIANT:
		     return SUPERTYPE_GAS_GIANT;
		case TYPE_PLANET_ASTEROID:
		case TYPE_PLANET_TERRESTRIAL:
		     return SUPERTYPE_ROCKY_PLANET;
		case TYPE_STARPORT_ORBITAL:
		case TYPE_STARPORT_SURFACE:
		     return SUPERTYPE_STARPORT;
		case TYPE_GRAVPOINT:
             return SUPERTYPE_NONE;
        default:
             Output("Warning: Invalid SuperBody Type found.\n");
             return SUPERTYPE_NONE;
	}
}

std::string SystemBody::GetAstroDescription() const
{
	PROFILE_SCOPED()
	switch (m_type) {
	case TYPE_BROWN_DWARF: return Lang::BROWN_DWARF;
	case TYPE_WHITE_DWARF: return Lang::WHITE_DWARF;
	case TYPE_STAR_M: return Lang::STAR_M;
	case TYPE_STAR_K: return Lang::STAR_K;
	case TYPE_STAR_G: return Lang::STAR_G;
	case TYPE_STAR_F: return Lang::STAR_F;
	case TYPE_STAR_A: return Lang::STAR_A;
	case TYPE_STAR_B: return Lang::STAR_B;
	case TYPE_STAR_O: return Lang::STAR_O;
	case TYPE_STAR_M_GIANT: return Lang::STAR_M_GIANT;
	case TYPE_STAR_K_GIANT: return Lang::STAR_K_GIANT;
	case TYPE_STAR_G_GIANT: return Lang::STAR_G_GIANT;
	case TYPE_STAR_F_GIANT: return Lang::STAR_AF_GIANT;
	case TYPE_STAR_A_GIANT: return Lang::STAR_AF_GIANT;
	case TYPE_STAR_B_GIANT: return Lang::STAR_B_GIANT;
	case TYPE_STAR_O_GIANT: return Lang::STAR_O_GIANT;
	case TYPE_STAR_M_SUPER_GIANT: return Lang::STAR_M_SUPER_GIANT;
	case TYPE_STAR_K_SUPER_GIANT: return Lang::STAR_K_SUPER_GIANT;
	case TYPE_STAR_G_SUPER_GIANT: return Lang::STAR_G_SUPER_GIANT;
	case TYPE_STAR_F_SUPER_GIANT: return Lang::STAR_AF_SUPER_GIANT;
	case TYPE_STAR_A_SUPER_GIANT: return Lang::STAR_AF_SUPER_GIANT;
	case TYPE_STAR_B_SUPER_GIANT: return Lang::STAR_B_SUPER_GIANT;
	case TYPE_STAR_O_SUPER_GIANT: return Lang::STAR_O_SUPER_GIANT;
	case TYPE_STAR_M_HYPER_GIANT: return Lang::STAR_M_HYPER_GIANT;
	case TYPE_STAR_K_HYPER_GIANT: return Lang::STAR_K_HYPER_GIANT;
	case TYPE_STAR_G_HYPER_GIANT: return Lang::STAR_G_HYPER_GIANT;
	case TYPE_STAR_F_HYPER_GIANT: return Lang::STAR_AF_HYPER_GIANT;
	case TYPE_STAR_A_HYPER_GIANT: return Lang::STAR_AF_HYPER_GIANT;
	case TYPE_STAR_B_HYPER_GIANT: return Lang::STAR_B_HYPER_GIANT;
	case TYPE_STAR_O_HYPER_GIANT: return Lang::STAR_O_HYPER_GIANT;
	case TYPE_STAR_M_WF: return Lang::STAR_M_WF;
	case TYPE_STAR_B_WF: return Lang::STAR_B_WF;
	case TYPE_STAR_O_WF: return Lang::STAR_O_WF;
	case TYPE_STAR_S_BH: return Lang::STAR_S_BH;
	case TYPE_STAR_IM_BH: return Lang::STAR_IM_BH;
	case TYPE_STAR_SM_BH: return Lang::STAR_SM_BH;
	case TYPE_PLANET_GAS_GIANT:
		if (m_mass > 800) return Lang::VERY_LARGE_GAS_GIANT;
		if (m_mass > 300) return Lang::LARGE_GAS_GIANT;
		if (m_mass > 80) return Lang::MEDIUM_GAS_GIANT;
		else return Lang::SMALL_GAS_GIANT;
	case TYPE_PLANET_ASTEROID: return Lang::ASTEROID;
	case TYPE_PLANET_TERRESTRIAL: {
		std::string s;
		if (m_mass > fixed(2,1)) s = Lang::MASSIVE;
		else if (m_mass > fixed(3,2)) s = Lang::LARGE;
		else if (m_mass < fixed(1,10)) s = Lang::TINY;
		else if (m_mass < fixed(1,5)) s = Lang::SMALL;

		if (m_volcanicity > fixed(7,10)) {
			if (s.size()) s += Lang::COMMA_HIGHLY_VOLCANIC;
			else s = Lang::HIGHLY_VOLCANIC;
		}

		if (m_volatileIces + m_volatileLiquid > fixed(4,5)) {
			if (m_volatileIces > m_volatileLiquid) {
				if (m_averageTemp < fixed(250)) {
					s += Lang::ICE_WORLD;
				} else s += Lang::ROCKY_PLANET;
			} else {
				if (m_averageTemp < fixed(250)) {
					s += Lang::ICE_WORLD;
				} else {
					s += Lang::OCEANICWORLD;
				}
			}
		} else if (m_volatileLiquid > fixed(2,5)){
			if (m_averageTemp > fixed(250)) {
				s += Lang::PLANET_CONTAINING_LIQUID_WATER;
			} else {
				s += Lang::PLANET_WITH_SOME_ICE;
			}
		} else if (m_volatileLiquid > fixed(1,5)){
			s += Lang::ROCKY_PLANET_CONTAINING_COME_LIQUIDS;
		} else {
			s += Lang::ROCKY_PLANET;
		}

		if (m_volatileGas < fixed(1,100)) {
			s += Lang::WITH_NO_SIGNIFICANT_ATMOSPHERE;
		} else {
			std::string thickness;
			if (m_volatileGas < fixed(1,10)) thickness = Lang::TENUOUS;
			else if (m_volatileGas < fixed(1,5)) thickness = Lang::THIN;
			else if (m_volatileGas < fixed(2,1)) {}
			else if (m_volatileGas < fixed(4,1)) thickness = Lang::THICK;
			else thickness = Lang::VERY_DENSE;

			if (m_atmosOxidizing > fixed(95,100)) {
				s += Lang::WITH_A+thickness+Lang::O2_ATMOSPHERE;
			} else if (m_atmosOxidizing > fixed(7,10)) {
				s += Lang::WITH_A+thickness+Lang::CO2_ATMOSPHERE;
			} else if (m_atmosOxidizing > fixed(65,100)) {
				s += Lang::WITH_A+thickness+Lang::CO_ATMOSPHERE;
			} else if (m_atmosOxidizing > fixed(55,100)) {
				s += Lang::WITH_A+thickness+Lang::CH4_ATMOSPHERE;
			} else if (m_atmosOxidizing > fixed(3,10)) {
				s += Lang::WITH_A+thickness+Lang::H_ATMOSPHERE;
			} else if (m_atmosOxidizing > fixed(2,10)) {
				s += Lang::WITH_A+thickness+Lang::HE_ATMOSPHERE;
			} else if (m_atmosOxidizing > fixed(15,100)) {
				s += Lang::WITH_A+thickness+Lang::AR_ATMOSPHERE;
			} else if (m_atmosOxidizing > fixed(1,10)) {
				s += Lang::WITH_A+thickness+Lang::S_ATMOSPHERE;
			} else {
				s += Lang::WITH_A+thickness+Lang::N_ATMOSPHERE;
			}
		}

		if (m_life > fixed(1,2)) {
			s += Lang::AND_HIGHLY_COMPLEX_ECOSYSTEM;
		} else if (m_life > fixed(1,10)) {
			s += Lang::AND_INDIGENOUS_PLANT_LIFE;
		} else if (m_life > fixed()) {
			s += Lang::AND_INDIGENOUS_MICROBIAL_LIFE;
		} else {
			s += ".";
		}

		return s;
	}
	case TYPE_STARPORT_ORBITAL:
		return Lang::ORBITAL_STARPORT;
	case TYPE_STARPORT_SURFACE:
		return Lang::STARPORT;
	case TYPE_GRAVPOINT:
    default:
        Output("Warning: Invalid Astro Body Description found.\n");
        return Lang::UNKNOWN;
	}
}

const char *SystemBody::GetIcon() const
{
	PROFILE_SCOPED()
	switch (m_type) {
	case TYPE_BROWN_DWARF: return "icons/object_brown_dwarf.png";
	case TYPE_WHITE_DWARF: return "icons/object_white_dwarf.png";
	case TYPE_STAR_M: return "icons/object_star_m.png";
	case TYPE_STAR_K: return "icons/object_star_k.png";
	case TYPE_STAR_G: return "icons/object_star_g.png";
	case TYPE_STAR_F: return "icons/object_star_f.png";
	case TYPE_STAR_A: return "icons/object_star_a.png";
	case TYPE_STAR_B: return "icons/object_star_b.png";
	case TYPE_STAR_O: return "icons/object_star_b.png"; //shares B graphic for now
	case TYPE_STAR_M_GIANT: return "icons/object_star_m_giant.png";
	case TYPE_STAR_K_GIANT: return "icons/object_star_k_giant.png";
	case TYPE_STAR_G_GIANT: return "icons/object_star_g_giant.png";
	case TYPE_STAR_F_GIANT: return "icons/object_star_f_giant.png";
	case TYPE_STAR_A_GIANT: return "icons/object_star_a_giant.png";
	case TYPE_STAR_B_GIANT: return "icons/object_star_b_giant.png";
	case TYPE_STAR_O_GIANT: return "icons/object_star_o.png"; // uses old O type graphic
	case TYPE_STAR_M_SUPER_GIANT: return "icons/object_star_m_super_giant.png";
	case TYPE_STAR_K_SUPER_GIANT: return "icons/object_star_k_super_giant.png";
	case TYPE_STAR_G_SUPER_GIANT: return "icons/object_star_g_super_giant.png";
	case TYPE_STAR_F_SUPER_GIANT: return "icons/object_star_g_super_giant.png"; //shares G graphic for now
	case TYPE_STAR_A_SUPER_GIANT: return "icons/object_star_a_super_giant.png";
	case TYPE_STAR_B_SUPER_GIANT: return "icons/object_star_b_super_giant.png";
	case TYPE_STAR_O_SUPER_GIANT: return "icons/object_star_b_super_giant.png";// uses B type graphic for now
	case TYPE_STAR_M_HYPER_GIANT: return "icons/object_star_m_hyper_giant.png";
	case TYPE_STAR_K_HYPER_GIANT: return "icons/object_star_k_hyper_giant.png";
	case TYPE_STAR_G_HYPER_GIANT: return "icons/object_star_g_hyper_giant.png";
	case TYPE_STAR_F_HYPER_GIANT: return "icons/object_star_f_hyper_giant.png";
	case TYPE_STAR_A_HYPER_GIANT: return "icons/object_star_a_hyper_giant.png";
	case TYPE_STAR_B_HYPER_GIANT: return "icons/object_star_b_hyper_giant.png";
	case TYPE_STAR_O_HYPER_GIANT: return "icons/object_star_b_hyper_giant.png";// uses B type graphic for now
	case TYPE_STAR_M_WF: return "icons/object_star_m_wf.png";
	case TYPE_STAR_B_WF: return "icons/object_star_b_wf.png";
	case TYPE_STAR_O_WF: return "icons/object_star_o_wf.png";
	case TYPE_STAR_S_BH: return "icons/object_star_bh.png";
	case TYPE_STAR_IM_BH: return "icons/object_star_smbh.png";
	case TYPE_STAR_SM_BH: return "icons/object_star_smbh.png";
	case TYPE_PLANET_GAS_GIANT:
		if (m_mass > 800) {
			if (m_averageTemp > 1000) return "icons/object_planet_large_gas_giant_hot.png";
			else return "icons/object_planet_large_gas_giant.png";
		}
		if (m_mass > 300) {
			if (m_averageTemp > 1000) return "icons/object_planet_large_gas_giant_hot.png";
			else return "icons/object_planet_large_gas_giant.png";
		}
		if (m_mass > 80) {
			if (m_averageTemp > 1000) return "icons/object_planet_medium_gas_giant_hot.png";
			else return "icons/object_planet_medium_gas_giant.png";
		}
		else {
			if (m_averageTemp > 1000) return "icons/object_planet_small_gas_giant_hot.png";
			else return "icons/object_planet_small_gas_giant.png";
		}
	case TYPE_PLANET_ASTEROID:
		return "icons/object_planet_asteroid.png";
	case TYPE_PLANET_TERRESTRIAL:
		if (m_volatileLiquid > fixed(7,10)) {
			if (m_averageTemp > 250) return "icons/object_planet_water.png";
			else return "icons/object_planet_ice.png";
		}
		if ((m_life > fixed(9,10)) &&
			(m_volatileGas > fixed(6,10))) return "icons/object_planet_life.png";
		if ((m_life > fixed(8,10)) &&
			(m_volatileGas > fixed(5,10))) return "icons/object_planet_life6.png";
		if ((m_life > fixed(7,10)) &&
			(m_volatileGas > fixed(45,100))) return "icons/object_planet_life7.png";
		if ((m_life > fixed(6,10)) &&
			(m_volatileGas > fixed(4,10))) return "icons/object_planet_life8.png";
		if ((m_life > fixed(5,10)) &&
			(m_volatileGas > fixed(3,10))) return "icons/object_planet_life4.png";
		if ((m_life > fixed(4,10)) &&
			(m_volatileGas > fixed(2,10))) return "icons/object_planet_life5.png";
		if ((m_life > fixed(1,10)) &&
			(m_volatileGas > fixed(2,10))) return "icons/object_planet_life2.png";
		if (m_life > fixed(1,10)) return "icons/object_planet_life3.png";
		if (m_mass < fixed(1,100)) return "icons/object_planet_dwarf.png";
		if (m_mass < fixed(1,10)) return "icons/object_planet_small.png";
		if ((m_volatileLiquid < fixed(1,10)) &&
			(m_volatileGas > fixed(1,5))) return "icons/object_planet_desert.png";

		if (m_volatileIces + m_volatileLiquid > fixed(3,5)) {
			if (m_volatileIces > m_volatileLiquid) {
				if (m_averageTemp < 250)	return "icons/object_planet_ice.png";
			} else {
				if (m_averageTemp > 250) {
					return "icons/object_planet_water.png";
				} else return "icons/object_planet_ice.png";
			}
		}

		if (m_volatileGas > fixed(1,2)) {
			if (m_atmosOxidizing < fixed(1,2)) {
				if (m_averageTemp > 300) return "icons/object_planet_methane3.png";
				else if (m_averageTemp > 250) return "icons/object_planet_methane2.png";
				else return "icons/object_planet_methane.png";
			} else {
				if (m_averageTemp > 300) return "icons/object_planet_co2_2.png";
				else if (m_averageTemp > 250) {
					if ((m_volatileLiquid > fixed(3,10)) && (m_volatileGas > fixed(2,10)))
						return "icons/object_planet_co2_4.png";
					else return "icons/object_planet_co2_3.png";
				} else return "icons/object_planet_co2.png";
			}
		}

		if ((m_volatileLiquid > fixed(1,10)) &&
		   (m_volatileGas < fixed(1,10))) return "icons/object_planet_ice.png";
		if (m_volcanicity > fixed(7,10)) return "icons/object_planet_volcanic.png";
		return "icons/object_planet_small.png";
		/*
		"icons/object_planet_water_n1.png"
		"icons/object_planet_life3.png"
		"icons/object_planet_life2.png"
		*/
	case TYPE_STARPORT_ORBITAL:
		return "icons/object_orbital_starport.png";
	case TYPE_GRAVPOINT:
	case TYPE_STARPORT_SURFACE:
    default:
        Output("Warning: Invalid body icon.\n");
		return 0;
	}
}

/*
 * Position a surface starport anywhere. Space.cpp::MakeFrameFor() ensures it
 * is on dry land (discarding this position if necessary)
 */
void PopulateStarSystemGenerator::PositionSettlementOnPlanet(SystemBody* sbody)
{
	PROFILE_SCOPED()
	Random r(sbody->GetSeed());
	// used for orientation on planet surface
	double r2 = r.Double(); 	// function parameter evaluation order is implementation-dependent
	double r1 = r.Double();		// can't put two rands in the same expression
	sbody->m_orbit.SetPlane(matrix3x3d::RotateZ(2*M_PI*r1) * matrix3x3d::RotateY(2*M_PI*r2));

	// store latitude and longitude to equivalent orbital parameters to
	// be accessible easier
	sbody->m_inclination = fixed(r1*10000,10000) + FIXED_PI/2;	// latitide
	sbody->m_orbitalOffset = FIXED_PI/2;						// longitude

}

double SystemBody::GetMaxChildOrbitalDistance() const
{
	PROFILE_SCOPED()
	double max = 0;
	for (unsigned int i=0; i<m_children.size(); i++) {
		if (m_children[i]->m_orbMax.ToDouble() > max) {
			max = m_children[i]->m_orbMax.ToDouble();
		}
	}
	return AU * max;
}

bool SystemBody::IsCoOrbitalWith(const SystemBody* other) const
{
	if(m_parent && m_parent->GetType()==SystemBody::TYPE_GRAVPOINT
	&& ((m_parent->m_children[0] == this && m_parent->m_children[1] == other)
	|| (m_parent->m_children[1] == this && m_parent->m_children[0] == other)))
		return true;
	return false;
}

bool SystemBody::IsCoOrbital() const
{
	if(m_parent	&& m_parent->GetType()==SystemBody::TYPE_GRAVPOINT && (m_parent->m_children[0] == this || m_parent->m_children[1] == this))
		return true;
	return false;
}

/*
 * These are the nice floating point surface temp calculating turds.
 *
static const double boltzman_const = 5.6704e-8;
static double calcEnergyPerUnitAreaAtDist(double star_radius, double star_temp, double object_dist)
{
	const double total_solar_emission = boltzman_const *
		star_temp*star_temp*star_temp*star_temp*
		4*M_PI*star_radius*star_radius;

	return total_solar_emission / (4*M_PI*object_dist*object_dist);
}

// bond albedo, not geometric
static double CalcSurfaceTemp(double star_radius, double star_temp, double object_dist, double albedo, double greenhouse)
{
	const double energy_per_meter2 = calcEnergyPerUnitAreaAtDist(star_radius, star_temp, object_dist);
	const double surface_temp = pow(energy_per_meter2*(1-albedo)/(4*(1-greenhouse)*boltzman_const), 0.25);
	return surface_temp;
}
*/
/*
 * Instead we use these butt-ugly overflow-prone spat of ejaculate:
 */
/*
 * star_radius in sol radii
 * star_temp in kelvin,
 * object_dist in AU
 * return energy per unit area in solar constants (1362 W/m^2 )
 */
static fixed calcEnergyPerUnitAreaAtDist(fixed star_radius, int star_temp, fixed object_dist)
{
	PROFILE_SCOPED()
	fixed temp = star_temp * fixed(1,5778);	//normalize to Sun's temperature
	const fixed total_solar_emission =
		temp*temp*temp*temp*star_radius*star_radius;

	return total_solar_emission / (object_dist*object_dist);	//return value in solar consts (overflow prevention)
}

//helper function, get branch of system tree from body all the way to the system's root and write it to path
static void getPathToRoot(const SystemBody* body, std::vector<const SystemBody*>& path)
{
	while(body)
	{
		path.push_back(body);
		body = body->GetParent();
	}
}

int StarSystemRandomGenerator::CalcSurfaceTemp(const SystemBody *primary, fixed distToPrimary, fixed albedo, fixed greenhouse)
{
	PROFILE_SCOPED()

	// accumulator seeded with current primary
	fixed energy_per_meter2 = calcEnergyPerUnitAreaAtDist(primary->m_radius, primary->m_averageTemp, distToPrimary);
	fixed dist;
	// find the other stars which aren't our parent star
	for( auto s : primary->GetStarSystem()->GetStars())
	{
		if(s != primary)
		{
			//get branches from body and star to system root
			std::vector<const SystemBody*> first_to_root;
			std::vector<const SystemBody*> second_to_root;
			getPathToRoot(primary, first_to_root);
			getPathToRoot(&(*s), second_to_root);
			std::vector<const SystemBody*>::reverse_iterator fit = first_to_root.rbegin();
			std::vector<const SystemBody*>::reverse_iterator sit = second_to_root.rbegin();
			while(sit!=second_to_root.rend() && fit!=first_to_root.rend() && (*sit)==(*fit))	//keep tracing both branches from system's root
			{																					//until they diverge
				sit++;
				fit++;
			}
			if (sit == second_to_root.rend()) sit--;
			if (fit == first_to_root.rend()) fit--;	//oops! one of the branches ends at lca, backtrack

			if((*fit)->IsCoOrbitalWith(*sit))	//planet is around one part of coorbiting pair, star is another.
			{
				dist = ((*fit)->GetOrbMaxAsFixed()+(*fit)->GetOrbMinAsFixed()) >> 1;	//binaries don't have fully initialized smaxes
			}
			else if((*sit)->IsCoOrbital())	//star is part of binary around which planet is (possibly indirectly) orbiting
			{
				bool inverted_ancestry = false;
				for(const SystemBody* body = (*sit); body; body = body->GetParent()) if(body == (*fit))
				{
					inverted_ancestry = true;	//ugly hack due to function being static taking planet's primary rather than being called from actual planet
					break;
				}
				if(inverted_ancestry) //primary is star's ancestor! Don't try to take its orbit (could probably be a gravpoint check at this point, but paranoia)
				{
					dist = distToPrimary;
				}
				else
				{
					dist = ((*fit)->GetOrbMaxAsFixed()+(*fit)->GetOrbMinAsFixed()) >> 1;	//simplified to planet orbiting stationary star
				}
			}
			else if((*fit)->IsCoOrbital())	//planet is around one part of coorbiting pair, star isn't coorbiting with it
			{
				dist = ((*sit)->GetOrbMaxAsFixed()+(*sit)->GetOrbMinAsFixed()) >> 1;	//simplified to star orbiting stationary planet
			}
			else		//neither is part of any binaries - hooray!
			{
				dist = (((*sit)->GetSemiMajorAxisAsFixed() - (*fit)->GetSemiMajorAxisAsFixed()).Abs() //avg of conjunction and opposition dist
					 + ((*sit)->GetSemiMajorAxisAsFixed() + (*fit)->GetSemiMajorAxisAsFixed())) >> 1;
			}
		}
		energy_per_meter2 += calcEnergyPerUnitAreaAtDist(s->m_radius, s->m_averageTemp, dist);
	}
	const fixed surface_temp_pow4 = energy_per_meter2 * (1-albedo)/(1-greenhouse);
	return (279*int(isqrt(isqrt((surface_temp_pow4.v)))))>>(fixed::FRAC/4); //multiplied by 279 to convert from Earth's temps to Kelvin
}

double SystemBody::CalcSurfaceGravity() const
{
	PROFILE_SCOPED()
	double r = GetRadius();
	if (r > 0.0) {
		return G * GetMass() / pow(r, 2);
	} else {
		return 0.0;
	}
}

SystemBody *StarSystem::GetBodyByPath(const SystemPath &path) const
{
	PROFILE_SCOPED()
	assert(m_path.IsSameSystem(path));
	assert(path.IsBodyPath());
	assert(path.bodyIndex < m_bodies.size());

	return m_bodies[path.bodyIndex].Get();
}

SystemPath StarSystem::GetPathOf(const SystemBody *sbody) const
{
	return sbody->GetPath();
}

SystemBody::SystemBody(const SystemPath& path, StarSystem *system) : m_parent(nullptr), m_path(path), m_seed(0), m_aspectRatio(1,1), m_orbMin(0),
	m_orbMax(0), m_rotationalPhaseAtStart(0), m_semiMajorAxis(0), m_eccentricity(0), m_orbitalOffset(0), m_axialTilt(0),
	m_inclination(0), m_averageTemp(0), m_type(TYPE_GRAVPOINT), m_isCustomBody(false), m_heightMapFractal(0), m_atmosDensity(0.0), m_system(system)
{
}

bool SystemBody::HasAtmosphere() const
{
	PROFILE_SCOPED()
	return (m_volatileGas > fixed(1,100));
}

bool SystemBody::IsScoopable() const
{
	PROFILE_SCOPED()
	return (GetSuperType() == SUPERTYPE_GAS_GIANT);
}

void StarSystemLegacyGeneratorBase::PickAtmosphere(SystemBody* sbody)
{
	PROFILE_SCOPED()
	/* Alpha value isn't real alpha. in the shader fog depth is determined
	 * by density*alpha, so that we can have very dense atmospheres
	 * without having them a big stinking solid color obscuring everything

	  These are our atmosphere colours, for terrestrial planets we use m_atmosOxidizing
	  for some variation to atmosphere colours
	 */
	switch (sbody->GetType()) {
		case SystemBody::TYPE_PLANET_GAS_GIANT:

			sbody->m_atmosColor = Color(255, 255, 255, 3);
			sbody->m_atmosDensity = 14.0;
			break;
		case SystemBody::TYPE_PLANET_ASTEROID:
			sbody->m_atmosColor = Color(0);
			sbody->m_atmosDensity = 0.0;
			break;
		default:
		case SystemBody::TYPE_PLANET_TERRESTRIAL:
			double r = 0, g = 0, b = 0;
			double atmo = sbody->GetAtmosOxidizing();
			if (sbody->GetVolatileGas() > 0.001) {
				if (atmo > 0.95) {
					// o2
					r = 1.0f + ((0.95f-atmo)*15.0f);
					g = 0.95f + ((0.95f-atmo)*10.0f);
					b = atmo*atmo*atmo*atmo*atmo;
				} else if (atmo > 0.7) {
					// co2
					r = atmo+0.05f;
					g = 1.0f + (0.7f-atmo);
					b = 0.8f;
				} else if (atmo > 0.65) {
					// co
					r = 1.0f + (0.65f-atmo);
					g = 0.8f;
					b = atmo + 0.25f;
				} else if (atmo > 0.55) {
					// ch4
					r = 1.0f + ((0.55f-atmo)*5.0);
					g = 0.35f - ((0.55f-atmo)*5.0);
					b = 0.4f;
				} else if (atmo > 0.3) {
					// h
					r = 1.0f;
					g = 1.0f;
					b = 1.0f;
				} else if (atmo > 0.2) {
					// he
					r = 1.0f;
					g = 1.0f;
					b = 1.0f;
				} else if (atmo > 0.15) {
					// ar
					r = 0.5f - ((0.15f-atmo)*5.0);
					g = 0.0f;
					b = 0.5f + ((0.15f-atmo)*5.0);
				} else if (atmo > 0.1) {
					// s
					r = 0.8f - ((0.1f-atmo)*4.0);
					g = 1.0f;
					b = 0.5f - ((0.1f-atmo)*10.0);
				} else {
					// n
					r = 1.0f;
					g = 1.0f;
					b = 1.0f;
				}
				sbody->m_atmosColor = Color(r*255, g*255, b*255, 255);
			} else {
				sbody->m_atmosColor = Color(0);
			}
			sbody->m_atmosDensity = sbody->GetVolatileGas();
			//Output("| Atmosphere :\n|      red   : [%f] \n|      green : [%f] \n|      blue  : [%f] \n", r, g, b);
			//Output("-------------------------------\n");
			break;
		/*default:
			sbody->m_atmosColor = Color(0.6f, 0.6f, 0.6f, 1.0f);
			sbody->m_atmosDensity = m_body->m_volatileGas.ToDouble();
			break;*/
	}
}

static const unsigned char RANDOM_RING_COLORS[][4] = {
	{ 156, 122,  98, 217 }, // jupiter-like
	{ 156, 122,  98, 217 }, // saturn-like
	{ 181, 173, 174, 217 }, // neptune-like
	{ 130, 122,  98, 217 }, // uranus-like
	{ 207, 122,  98, 217 }  // brown dwarf-like
};

void StarSystemLegacyGeneratorBase::PickRings(SystemBody* sbody, bool forceRings)
{
	PROFILE_SCOPED()
	sbody->m_rings.minRadius = fixed();
	sbody->m_rings.maxRadius = fixed();
	sbody->m_rings.baseColor = Color(255,255,255,255);

	if (sbody->GetType() == SystemBody::TYPE_PLANET_GAS_GIANT) {
		Random ringRng(sbody->GetSeed() + 965467);

		// today's forecast: 50% chance of rings
		double rings_die = ringRng.Double();
		if (forceRings || (rings_die < 0.5)) {
			const unsigned char * const baseCol
				= RANDOM_RING_COLORS[ringRng.Int32(COUNTOF(RANDOM_RING_COLORS))];
			sbody->m_rings.baseColor.r = Clamp(baseCol[0] + ringRng.Int32(-20,20), 0, 255);
			sbody->m_rings.baseColor.g = Clamp(baseCol[1] + ringRng.Int32(-20,20), 0, 255);
			sbody->m_rings.baseColor.b = Clamp(baseCol[2] + ringRng.Int32(-20,10), 0, 255);
			sbody->m_rings.baseColor.a = Clamp(baseCol[3] + ringRng.Int32(-5,5), 0, 255);

			// from wikipedia: http://en.wikipedia.org/wiki/Roche_limit
			// basic Roche limit calculation assuming a rigid satellite
			// d = R (2 p_M / p_m)^{1/3}
			//
			// where R is the radius of the primary, p_M is the density of
			// the primary and p_m is the density of the satellite
			//
			// I assume a satellite density of 500 kg/m^3
			// (which Wikipedia says is an average comet density)
			//
			// also, I can't be bothered to think about unit conversions right now,
			// so I'm going to ignore the real density of the primary and take it as 1100 kg/m^3
			// (note: density of Saturn is ~687, Jupiter ~1,326, Neptune ~1,638, Uranus ~1,318)
			//
			// This gives: d = 1.638642 * R
			fixed innerMin = fixed(110, 100);
			fixed innerMax = fixed(145, 100);
			fixed outerMin = fixed(150, 100);
			fixed outerMax = fixed(168642, 100000);

			sbody->m_rings.minRadius = innerMin + (innerMax - innerMin)*ringRng.Fixed();
			sbody->m_rings.maxRadius = outerMin + (outerMax - outerMin)*ringRng.Fixed();
		}
	}
}

// Calculate parameters used in the atmospheric model for shaders
SystemBody::AtmosphereParameters SystemBody::CalcAtmosphereParams() const
{
	PROFILE_SCOPED()
	AtmosphereParameters params;

	double atmosDensity;

	GetAtmosphereFlavor(&params.atmosCol, &atmosDensity);
	// adjust global atmosphere opacity
	atmosDensity *= 1e-5;

	params.atmosDensity = static_cast<float>(atmosDensity);

	// Calculate parameters used in the atmospheric model for shaders
	// Isothermal atmospheric model
	// See http://en.wikipedia.org/wiki/Atmospheric_pressure#Altitude_atmospheric_pressure_variation
	// This model features an exponential decrease in pressure and density with altitude.
	// The scale height is 1/the exponential coefficient.

	// The equation for pressure is:
	// Pressure at height h = Pressure surface * e^((-Mg/RT)*h)

	// calculate (inverse) atmosphere scale height
	// The formula for scale height is:
	// h = RT / Mg
	// h is height above the surface in meters
	// R is the universal gas constant
	// T is the surface temperature in Kelvin
	// g is the gravity in m/s^2
	// M is the molar mass of air in kg/mol

	// calculate gravity
	// radius of the planet
	const double radiusPlanet_in_m = (m_radius.ToDouble()*EARTH_RADIUS);
	const double massPlanet_in_kg = (m_mass.ToDouble()*EARTH_MASS);
	const double g = G*massPlanet_in_kg/(radiusPlanet_in_m*radiusPlanet_in_m);

	double T = static_cast<double>(m_averageTemp);

	// XXX hack to avoid issues with sysgen giving 0 temps
	// temporary as part of sysgen needs to be rewritten before the proper fix can be used
	if (T < 1)
		T = 165;

	// We have two kinds of atmosphere: Earth-like and gas giant (hydrogen/helium)
	const double M = m_type == TYPE_PLANET_GAS_GIANT ? 0.0023139903 : 0.02897f; // in kg/mol

	float atmosScaleHeight = static_cast<float>(GAS_CONSTANT_R*T/(M*g));

	// min of 2.0 corresponds to a scale height of 1/20 of the planet's radius,
	params.atmosInvScaleHeight = std::max(20.0f, static_cast<float>(GetRadius() / atmosScaleHeight));
	// integrate atmospheric density between surface and this radius. this is 10x the scale
	// height, which should be a height at which the atmospheric density is negligible
	params.atmosRadius = 1.0f + static_cast<float>(10.0f * atmosScaleHeight) / GetRadius();

	params.planetRadius = static_cast<float>(radiusPlanet_in_m);

	return params;
}

/*
 * As my excellent comrades have pointed out, choices that depend on floating
 * point crap will result in different universes on different platforms.
 *
 * We must be sneaky and avoid floating point in these places.
 */
StarSystem::StarSystem(const SystemPath &path, StarSystemCache* cache, Random& rand) : m_path(path.SystemOnly()), m_numStars(0), m_isCustom(false),
	m_faction(nullptr), m_unexplored(false), m_econType(GalacticEconomy::ECON_MINING), m_seed(0), m_cache(cache)
{
	PROFILE_SCOPED()
	memset(m_tradeLevel, 0, sizeof(m_tradeLevel));
}

#ifdef DEBUG_DUMP
struct thing_t {
	SystemBody* obj;
	vector3d pos;
	vector3d vel;
};
void StarSystem::Dump()
{
	std::vector<SystemBody*> obj_stack;
	std::vector<vector3d> pos_stack;
	std::vector<thing_t> output;

	SystemBody *obj = m_rootBody;
	vector3d pos = vector3d(0.0);

	while (obj) {
		vector3d p2 = pos;
		if (obj->m_parent) {
			p2 = pos + obj->m_orbit.OrbitalPosAtTime(1.0);
			pos = pos + obj->m_orbit.OrbitalPosAtTime(0.0);
		}

		if ((obj->GetType() != SystemBody::TYPE_GRAVPOINT) &&
		    (obj->GetSuperType() != SystemBody::SUPERTYPE_STARPORT)) {
			struct thing_t t;
			t.obj = obj;
			t.pos = pos;
			t.vel = (p2-pos);
			output.push_back(t);
		}
		for (std::vector<SystemBody*>::iterator i = obj->m_children.begin();
				i != obj->m_children.end(); ++i) {
			obj_stack.push_back(*i);
			pos_stack.push_back(pos);
		}
		if (obj_stack.size() == 0) break;
		pos = pos_stack.back();
		obj = obj_stack.back();
		pos_stack.pop_back();
		obj_stack.pop_back();
	}

	FILE *f = fopen("starsystem.dump", "w");
	fprintf(f, "%lu bodies\n", output.size());
	fprintf(f, "0 steps\n");
	for (std::vector<thing_t>::iterator i = output.begin();
			i != output.end(); ++i) {
		fprintf(f, "B:%lf,%lf:%lf,%lf,%lf,%lf:%lf:%d:%lf,%lf,%lf\n",
				(*i).pos.x, (*i).pos.y, (*i).pos.z,
				(*i).vel.x, (*i).vel.y, (*i).vel.z,
				(*i).obj->GetMass(), 0,
				1.0, 1.0, 1.0);
	}
	fclose(f);
	Output("Junk dumped to starsystem.dump\n");
}
#endif /* DEBUG_DUMP */

/*
 * http://en.wikipedia.org/wiki/Hill_sphere
 */
fixed StarSystemLegacyGeneratorBase::CalcHillRadius(SystemBody* sbody) const
{
	PROFILE_SCOPED()
	if (sbody->GetSuperType() <= SystemBody::SUPERTYPE_STAR) {
		return fixed();
	} else {
		// playing with precision since these numbers get small
		// masses in earth masses
		fixedf<32> mprimary = sbody->GetParent()->GetMassInEarths();

		fixedf<48> a = sbody->GetSemiMajorAxisAsFixed();
		fixedf<48> e = sbody->GetEccentricityAsFixed();

		return fixed(a * (fixedf<48>(1,1)-e) *
				fixedf<48>::CubeRootOf(fixedf<48>(
						sbody->GetMassAsFixed() / (fixedf<32>(3,1)*mprimary))));

		//fixed hr = semiMajorAxis*(fixed(1,1) - eccentricity) *
		//  fixedcuberoot(mass / (3*mprimary));
	}
}

/*
 * For moons distance from star is not orbMin, orbMax.
 */
const SystemBody* StarSystemRandomGenerator::FindStarAndTrueOrbitalRange(const SystemBody *planet, fixed &orbMin_, fixed &orbMax_) const
{
	PROFILE_SCOPED()
	const SystemBody *star = planet->GetParent();

	assert(star);

	/* while not found star yet.. */
	while (star->GetSuperType() > SystemBody::SUPERTYPE_STAR) {
		planet = star;
		star = star->GetParent();
	}

	orbMin_ = planet->GetOrbMinAsFixed();
	orbMax_ = planet->GetOrbMaxAsFixed();
	return star;
}

void StarSystemRandomGenerator::PickPlanetType(SystemBody *sbody, Random &rand)
{
	PROFILE_SCOPED()
	fixed albedo;
	fixed greenhouse;

	fixed minDistToStar, maxDistToStar, averageDistToStar;
	const SystemBody* star = FindStarAndTrueOrbitalRange(sbody, minDistToStar, maxDistToStar);
	averageDistToStar = (minDistToStar+maxDistToStar)>>1;

	/* first calculate blackbody temp (no greenhouse effect, zero albedo) */
	int bbody_temp = CalcSurfaceTemp(star, averageDistToStar, albedo, greenhouse);

	sbody->m_averageTemp = bbody_temp;

	static const fixed ONEEUMASS = fixed::FromDouble(1);
	static const fixed TWOHUNDREDEUMASSES = fixed::FromDouble(200.0);
	// We get some more fractional bits for small bodies otherwise we can easily end up with 0 radius which breaks stuff elsewhere
	//
	// AndyC - Updated to use the empirically gathered data from this site:
	// http://phl.upr.edu/library/notes/standardmass-radiusrelationforexoplanets
	// but we still limit at the lowest end
	if (sbody->GetMassAsFixed() <= fixed(1,1)) {
		sbody->m_radius = fixed(fixedf<48>::CubeRootOf(fixedf<48>(sbody->GetMassAsFixed())));
	} else if( sbody->GetMassAsFixed() < ONEEUMASS ) {
		// smaller than 1 Earth mass is almost certainly a rocky body
		sbody->m_radius = fixed::FromDouble(pow( sbody->GetMassAsFixed().ToDouble(), 0.3 ));
	} else if( sbody->GetMassAsFixed() < TWOHUNDREDEUMASSES ) {
		// from 1 EU to 200 they transition from Earth-like rocky bodies, through Ocean worlds and on to Gas Giants
		sbody->m_radius = fixed::FromDouble(pow( sbody->GetMassAsFixed().ToDouble(), 0.5 ));
	} else {
		// Anything bigger than 200 EU masses is a Gas Giant or bigger but the density changes to decrease from here on up...
		sbody->m_radius = fixed::FromDouble( 22.6 * (1.0/pow(sbody->GetMassAsFixed().ToDouble(), double(0.0886))) );
	}
	// enforce minimum size of 10km
	sbody->m_radius = std::max(sbody->GetRadiusAsFixed(), fixed(1,630));

	if (sbody->GetParent()->GetType() <= SystemBody::TYPE_STAR_MAX) {
		// get it from the table now rather than setting it on stars/gravpoints as
		// currently nothing else needs them to have metallicity
		sbody->m_metallicity = starMetallicities[sbody->GetParent()->GetType()] * rand.Fixed();
	} else {
		// this assumes the parent's parent is a star/gravpoint, which is currently always true
		sbody->m_metallicity = starMetallicities[sbody->GetParent()->GetParent()->GetType()] * rand.Fixed();
	}

	// harder to be volcanic when you are tiny (you cool down)
	sbody->m_volcanicity = std::min(fixed(1,1), sbody->GetMassAsFixed()) * rand.Fixed();
	sbody->m_atmosOxidizing = rand.Fixed();
	sbody->m_life = fixed();
	sbody->m_volatileGas = fixed();
	sbody->m_volatileLiquid = fixed();
	sbody->m_volatileIces = fixed();

	// pick body type
	if (sbody->GetMassAsFixed() > 317*13) {
		// more than 13 jupiter masses can fuse deuterium - is a brown dwarf
		sbody->m_type = SystemBody::TYPE_BROWN_DWARF;
		sbody->m_averageTemp = sbody->GetAverageTemp() + rand.Int32(starTypeInfo[sbody->GetType()].tempMin, starTypeInfo[sbody->GetType()].tempMax);
		// prevent mass exceeding 65 jupiter masses or so, when it becomes a star
		// XXX since TYPE_BROWN_DWARF is supertype star, mass is now in
		// solar masses. what a fucking mess
		sbody->m_mass = std::min(sbody->GetMassAsFixed(), fixed(317*65, 1)) / SUN_MASS_TO_EARTH_MASS;
		//Radius is too high as it now uses the planetary calculations to work out radius (Cube root of mass)
		// So tell it to use the star data instead:
		sbody->m_radius = fixed(rand.Int32(starTypeInfo[sbody->GetType()].radius[0], starTypeInfo[sbody->GetType()].radius[1]), 100);
	} else if (sbody->GetMassAsFixed() > 6) {
		sbody->m_type = SystemBody::TYPE_PLANET_GAS_GIANT;
	} else if (sbody->GetMassAsFixed() > fixed(1, 15000)) {
		sbody->m_type = SystemBody::TYPE_PLANET_TERRESTRIAL;

		fixed amount_volatiles = fixed(2,1)*rand.Fixed();
		if (rand.Int32(3)) amount_volatiles *= sbody->GetMassAsFixed();
		// total atmosphere loss
		if (rand.Fixed() > sbody->GetMassAsFixed()) amount_volatiles = fixed();

		//Output("Amount volatiles: %f\n", amount_volatiles.ToFloat());
		// fudge how much of the volatiles are in which state
		greenhouse = fixed();
		albedo = fixed();
		// CO2 sublimation
		if (sbody->GetAverageTemp() > 195) greenhouse += amount_volatiles * fixed(1,3);
		else albedo += fixed(2,6);
		// H2O liquid
		if (sbody->GetAverageTemp() > 273) greenhouse += amount_volatiles * fixed(1,5);
		else albedo += fixed(3,6);
		// H2O boils
		if (sbody->GetAverageTemp() > 373) greenhouse += amount_volatiles * fixed(1,3);

		if(greenhouse > fixed(7,10)) { // never reach 1, but 1/(1-greenhouse) still grows
			greenhouse *= greenhouse;
			greenhouse *= greenhouse;
			greenhouse = greenhouse / (greenhouse + fixed(32,311));
		}

		sbody->m_averageTemp = CalcSurfaceTemp(star, averageDistToStar, albedo, greenhouse);

		const fixed proportion_gas = sbody->GetAverageTemp() / (fixed(100,1) + sbody->GetAverageTemp());
		sbody->m_volatileGas = proportion_gas * amount_volatiles;

		const fixed proportion_liquid = (fixed(1,1)-proportion_gas) * (sbody->GetAverageTemp() / (fixed(50,1) + sbody->GetAverageTemp()));
		sbody->m_volatileLiquid = proportion_liquid * amount_volatiles;

		const fixed proportion_ices = fixed(1,1) - (proportion_gas + proportion_liquid);
		sbody->m_volatileIces = proportion_ices * amount_volatiles;

		//Output("temp %dK, gas:liquid:ices %f:%f:%f\n", averageTemp, proportion_gas.ToFloat(),
		//		proportion_liquid.ToFloat(), proportion_ices.ToFloat());

		if ((sbody->GetVolatileLiquidAsFixed() > fixed()) &&
		    (sbody->GetAverageTemp() > CELSIUS-60) &&
		    (sbody->GetAverageTemp() < CELSIUS+200))
		{
			// try for life
			int minTemp = CalcSurfaceTemp(star, maxDistToStar, albedo, greenhouse);
			int maxTemp = CalcSurfaceTemp(star, minDistToStar, albedo, greenhouse);

			if ((minTemp > CELSIUS-10) && (minTemp < CELSIUS+90) &&	//removed explicit checks for star type (also BD and WD seem to have slight chance of having life around them)
			    (maxTemp > CELSIUS-10) && (maxTemp < CELSIUS+90))	//TODO: ceiling based on actual boiling point on the planet, not in 1atm
			{
			    fixed maxMass, lifeMult, allowedMass(1,2);
			    allowedMass += 2;
				for( auto s : sbody->GetStarSystem()->GetStars() ) {	//find the most massive star, mass is tied to lifespan
					maxMass = maxMass < s->GetMassAsFixed() ? s->GetMassAsFixed() : maxMass;	//this automagically eliminates O, B and so on from consideration
				}	//handy calculator: http://www.asc-csa.gc.ca/eng/educators/resources/astronomy/module2/calculator.asp
				if(maxMass < allowedMass) {	//system could have existed long enough for life to form (based on Sol)
					lifeMult = allowedMass - maxMass;
				}
				sbody->m_life = lifeMult * rand.Fixed();
			}
		}
	} else {
		sbody->m_type = SystemBody::TYPE_PLANET_ASTEROID;
	}

	// Tidal lock for planets close to their parents:
	//		http://en.wikipedia.org/wiki/Tidal_locking
	//
	//		Formula: time ~ semiMajorAxis^6 * radius / mass / parentMass^2
	//
	//		compared to Earth's Moon
	static fixed MOON_TIDAL_LOCK = fixed(6286,1);
	fixed invTidalLockTime = fixed(1,1);

	// fine-tuned not to give overflows, order of evaluation matters!
	if (sbody->GetParent()->GetType() <= SystemBody::TYPE_STAR_MAX) {
		invTidalLockTime /= (sbody->GetSemiMajorAxisAsFixed() * sbody->GetSemiMajorAxisAsFixed());
		invTidalLockTime *= sbody->GetMassAsFixed();
		invTidalLockTime /= (sbody->GetSemiMajorAxisAsFixed() * sbody->GetSemiMajorAxisAsFixed());
		invTidalLockTime *= sbody->GetParent()->GetMassAsFixed()*sbody->GetParent()->GetMassAsFixed();
		invTidalLockTime /= sbody->GetRadiusAsFixed();
		invTidalLockTime /= (sbody->GetSemiMajorAxisAsFixed() * sbody->GetSemiMajorAxisAsFixed())*MOON_TIDAL_LOCK;
	} else {
		invTidalLockTime /= (sbody->GetSemiMajorAxisAsFixed() * sbody->GetSemiMajorAxisAsFixed())*SUN_MASS_TO_EARTH_MASS;
		invTidalLockTime *= sbody->GetMassAsFixed();
		invTidalLockTime /= (sbody->GetSemiMajorAxisAsFixed() * sbody->GetSemiMajorAxisAsFixed())*SUN_MASS_TO_EARTH_MASS;
		invTidalLockTime *= sbody->GetParent()->GetMassAsFixed()*sbody->GetParent()->GetMassAsFixed();
		invTidalLockTime /= sbody->GetRadiusAsFixed();
		invTidalLockTime /= (sbody->GetSemiMajorAxisAsFixed() * sbody->GetSemiMajorAxisAsFixed())*MOON_TIDAL_LOCK;
	}
	//Output("tidal lock of %s: %.5f, a %.5f R %.4f mp %.3f ms %.3f\n", name.c_str(),
	//		invTidalLockTime.ToFloat(), semiMajorAxis.ToFloat(), radius.ToFloat(), parent->mass.ToFloat(), mass.ToFloat());

	if(invTidalLockTime > 10) { // 10x faster than Moon, no chance not to be tidal-locked
		sbody->m_rotationPeriod = fixed(int(round(sbody->GetOrbit().Period())),3600*24);
		sbody->m_axialTilt = sbody->GetInclinationAsFixed();
	} else if(invTidalLockTime > fixed(1,100)) { // rotation speed changed in favour of tidal lock
		// XXX: there should be some chance the satellite was captured only recenly and ignore this
		//		I'm ommiting that now, I do not want to change the Universe by additional rand call.

		fixed lambda = invTidalLockTime/(fixed(1,20)+invTidalLockTime);
		sbody->m_rotationPeriod = (1-lambda)*sbody->GetRotationPeriodAsFixed()+ lambda*sbody->GetOrbit().Period()/3600/24;
		sbody->m_axialTilt = (1-lambda)*sbody->GetAxialTiltAsFixed() + lambda*sbody->GetInclinationAsFixed();
	} // else .. nothing happens to the satellite

    PickAtmosphere(sbody);
	PickRings(sbody);
}

/*
 * Set natural resources, tech level, industry strengths and population levels
 */
void PopulateStarSystemGenerator::PopulateStage1(SystemBody* sbody, StarSystem::GeneratorAPI *system, fixed &outTotalPop)
{
	PROFILE_SCOPED()
	for (auto child : sbody->GetChildren()) {
		PopulateStage1(child, system, outTotalPop);
	}

	// unexplored systems have no population (that we know about)
	if (system->GetUnexplored()) {
		sbody->m_population = outTotalPop = fixed();
		return;
	}

	// grav-points have no population themselves
	if (sbody->GetType() == SystemBody::TYPE_GRAVPOINT) {
		sbody->m_population = fixed();
		return;
	}

	Uint32 _init[6] = { system->GetPath().systemIndex, Uint32(system->GetPath().sectorX),
			Uint32(system->GetPath().sectorY), Uint32(system->GetPath().sectorZ), UNIVERSE_SEED, Uint32(sbody->GetSeed()) };

	Random rand;
	rand.seed(_init, 6);

	RefCountedPtr<Random> namerand(new Random);
	namerand->seed(_init, 6);

	sbody->m_population = fixed();

	/* Bad type of planet for settlement */
	if ((sbody->GetAverageTemp() > CELSIUS+100) || (sbody->GetAverageTemp() < 100) ||
	    (sbody->GetType() != SystemBody::TYPE_PLANET_TERRESTRIAL && sbody->GetType() != SystemBody::TYPE_PLANET_ASTEROID)) {

        // orbital starports should carry a small amount of population
        if (sbody->GetType() == SystemBody::TYPE_STARPORT_ORBITAL) {
			sbody->m_population = fixed(1,100000);
			outTotalPop += sbody->m_population;
        }

		return;
	}

	sbody->m_agricultural = fixed();

	if (sbody->GetLifeAsFixed() > fixed(9,10)) {
		sbody->m_agricultural = Clamp(fixed(1,1) - fixed(CELSIUS+25-sbody->GetAverageTemp(), 40), fixed(), fixed(1,1));
		system->SetAgricultural(system->GetAgricultural() + 2*sbody->m_agricultural);
	} else if (sbody->GetLifeAsFixed() > fixed(1,2)) {
		sbody->m_agricultural = Clamp(fixed(1,1) - fixed(CELSIUS+30-sbody->GetAverageTemp(), 50), fixed(), fixed(1,1));
		system->SetAgricultural(system->GetAgricultural() + 1*sbody->m_agricultural);
	} else {
		// don't bother populating crap planets
		if (sbody->GetMetallicityAsFixed() < fixed(5,10) &&
			sbody->GetMetallicityAsFixed() < (fixed(1,1) - system->GetHumanProx())) return;
	}

	const int NUM_CONSUMABLES = 10;
	const GalacticEconomy::Commodity consumables[NUM_CONSUMABLES] = {
		GalacticEconomy::Commodity::AIR_PROCESSORS,
		GalacticEconomy::Commodity::GRAIN,
		GalacticEconomy::Commodity::FRUIT_AND_VEG,
		GalacticEconomy::Commodity::ANIMAL_MEAT,
		GalacticEconomy::Commodity::LIQUOR,
		GalacticEconomy::Commodity::CONSUMER_GOODS,
		GalacticEconomy::Commodity::MEDICINES,
		GalacticEconomy::Commodity::HAND_WEAPONS,
		GalacticEconomy::Commodity::NARCOTICS,
		GalacticEconomy::Commodity::LIQUID_OXYGEN
	};

	/* Commodities we produce (mining and agriculture) */

	for (int i = 1; i < GalacticEconomy::COMMODITY_COUNT; i++) {
		const GalacticEconomy::CommodityInfo &info = GalacticEconomy::COMMODITY_DATA[i];

		fixed affinity = fixed(1,1);
		if (info.econType & GalacticEconomy::ECON_AGRICULTURE) {
			affinity *= 2*sbody->GetAgriculturalAsFixed();
		}
		if (info.econType & GalacticEconomy::ECON_INDUSTRY) affinity *= system->GetIndustrial();
		// make industry after we see if agriculture and mining are viable
		if (info.econType & GalacticEconomy::ECON_MINING) {
			affinity *= sbody->GetMetallicityAsFixed();
		}
		affinity *= rand.Fixed();
		// producing consumables is wise
		for (int j=0; j<NUM_CONSUMABLES; j++) {
			if (GalacticEconomy::Commodity(i) == consumables[j]) {
				affinity *= 2;
				break;
			}
		}
		assert(affinity >= 0);
		/* workforce... */
		sbody->m_population += affinity * system->GetHumanProx();

		int howmuch = (affinity * 256).ToInt32();

		system->AddTradeLevel(GalacticEconomy::Commodity(i), -2*howmuch);
		for (int j=0; j < GalacticEconomy::CommodityInfo::MAX_ECON_INPUTS; ++j) {
			if (info.inputs[j] == GalacticEconomy::Commodity::NONE) continue;
			system->AddTradeLevel(GalacticEconomy::Commodity(info.inputs[j]), howmuch);
		}
	}

	if (!system->HasCustomBodies() && sbody->GetPopulationAsFixed() > 0)
		sbody->m_name = Pi::luaNameGen->BodyName(sbody, namerand);

	// Add a bunch of things people consume
	for (int i=0; i<NUM_CONSUMABLES; i++) {
		GalacticEconomy::Commodity t = consumables[i];
		if (sbody->GetLifeAsFixed() > fixed(1,2)) {
			// life planets can make this jizz probably
			if ((t == GalacticEconomy::Commodity::AIR_PROCESSORS) ||
			    (t == GalacticEconomy::Commodity::LIQUID_OXYGEN) ||
			    (t == GalacticEconomy::Commodity::GRAIN) ||
			    (t == GalacticEconomy::Commodity::FRUIT_AND_VEG) ||
			    (t == GalacticEconomy::Commodity::ANIMAL_MEAT)) {
				continue;
			}
		}
		system->AddTradeLevel(GalacticEconomy::Commodity(t), rand.Int32(32,128));
	}
	// well, outdoor worlds should have way more people
	sbody->m_population = fixed(1,10)*sbody->m_population + sbody->m_population*sbody->GetAgriculturalAsFixed();

//	Output("%s: pop %.3f billion\n", name.c_str(), sbody->m_population.ToFloat());

	outTotalPop += sbody->GetPopulationAsFixed();
}

static bool check_unique_station_name(const std::string & name, const StarSystem * system) {
	PROFILE_SCOPED()
	bool ret = true;
	for (const SystemBody *station : system->GetSpaceStations())
		if (station->GetName() == name) {
			ret = false;
			break;
		}
	return ret;
}

static std::string gen_unique_station_name(SystemBody *sp, const StarSystem *system, RefCountedPtr<Random> &namerand) {
	PROFILE_SCOPED()
	std::string name;
	do {
		name = Pi::luaNameGen->BodyName(sp, namerand);
	} while (!check_unique_station_name(name, system));
	return name;
}

void PopulateStarSystemGenerator::PopulateAddStations(SystemBody* sbody, StarSystem::GeneratorAPI *system)
{
	PROFILE_SCOPED()
	for (auto child : sbody->GetChildren())
		PopulateAddStations(child, system);

	Uint32 _init[6] = { system->GetPath().systemIndex, Uint32(system->GetPath().sectorX),
			Uint32(system->GetPath().sectorY), Uint32(system->GetPath().sectorZ), sbody->GetSeed(), UNIVERSE_SEED };

	Random rand;
	rand.seed(_init, 6);

	RefCountedPtr<Random> namerand(new Random);
	namerand->seed(_init, 6);

	if (sbody->GetPopulationAsFixed() < fixed(1,1000)) return;

	fixed orbMaxS = fixed(1,4)*CalcHillRadius(sbody);
	fixed orbMinS = 4 * sbody->GetRadiusAsFixed() * AU_EARTH_RADIUS;
	if (sbody->GetNumChildren()) orbMaxS = std::min(orbMaxS, fixed(1,2) * sbody->GetChildren()[0]->GetOrbMinAsFixed());

	// starports - orbital
	fixed pop = sbody->GetPopulationAsFixed() + rand.Fixed();
	if( orbMinS < orbMaxS )
	{
		pop -= rand.Fixed();
		Uint32 NumToMake = 0;
		while(pop >= 0) {
			++NumToMake;
			pop -= rand.Fixed();
		}
		for( Uint32 i=0; i<NumToMake; i++ ) {
			SystemBody *sp = system->NewBody();
			sp->m_type = SystemBody::TYPE_STARPORT_ORBITAL;
			sp->m_seed = rand.Int32();
			sp->m_parent = sbody;
			sp->m_rotationPeriod = fixed(1,3600);
			sp->m_averageTemp = sbody->GetAverageTemp();
			sp->m_mass = 0;

			// place stations between min and max orbits to reduce the number of extremely close/fast orbits
			sp->m_semiMajorAxis = orbMinS + ((orbMaxS - orbMinS) / 4);
			sp->m_eccentricity = fixed();
			sp->m_axialTilt = fixed();

			sp->m_orbit.SetShapeAroundPrimary(sp->GetSemiMajorAxisAsFixed().ToDouble()*AU, sbody->GetMassAsFixed().ToDouble() * EARTH_MASS, 0.0);
			if (NumToMake > 1) {
				sp->m_orbit.SetPlane(matrix3x3d::RotateZ(double(i) * (M_PI / double(NumToMake-1))));
			} else {
				sp->m_orbit.SetPlane(matrix3x3d::Identity());
			}

			sp->m_inclination = fixed();
			sbody->m_children.insert(sbody->m_children.begin(), sp);
			system->AddSpaceStation(sp);
			sp->m_orbMin = sp->GetSemiMajorAxisAsFixed();
			sp->m_orbMax = sp->GetSemiMajorAxisAsFixed();

			sp->m_name = gen_unique_station_name(sp, system, namerand);
		}
	}
	// starports - surface
	// give it a fighting chance of having a decent number of starports (*3)
	pop = sbody->GetPopulationAsFixed() + (rand.Fixed() * 3);
	int max = 6;
	while (max-- > 0) {
		pop -= rand.Fixed();
		if (pop < 0) break;

		SystemBody *sp = system->NewBody();
		sp->m_type = SystemBody::TYPE_STARPORT_SURFACE;
		sp->m_seed = rand.Int32();
		sp->m_parent = sbody;
		sp->m_averageTemp = sbody->GetAverageTemp();
		sp->m_mass = 0;
		sp->m_name = gen_unique_station_name(sp, system, namerand);
		memset(&sp->m_orbit, 0, sizeof(Orbit));
		PositionSettlementOnPlanet(sp);
		sbody->m_children.insert(sbody->m_children.begin(), sp);
		system->AddSpaceStation(sp);
	}

	// garuantee that there is always a star port on a populated world
	if( !system->HasSpaceStations() )
	{
		SystemBody *sp = system->NewBody();
		sp->m_type = SystemBody::TYPE_STARPORT_SURFACE;
		sp->m_seed = rand.Int32();
		sp->m_parent = sbody;
		sp->m_averageTemp = sbody->m_averageTemp;
		sp->m_mass = 0;
		sp->m_name = gen_unique_station_name(sp, system, namerand);
		memset(&sp->m_orbit, 0, sizeof(Orbit));
		PositionSettlementOnPlanet(sp);
		sbody->m_children.insert(sbody->m_children.begin(), sp);
		system->AddSpaceStation(sp);
	}
}

void SystemBody::Dump(FILE* file, const char* indent) const
{
	fprintf(file, "%sSystemBody(%d,%d,%d,%u,%u) : %s/%s %s{\n", indent, m_path.sectorX, m_path.sectorY, m_path.sectorZ, m_path.systemIndex,
		m_path.bodyIndex, EnumStrings::GetString("BodySuperType", GetSuperType()), EnumStrings::GetString("BodyType", m_type),
		m_isCustomBody ? "CUSTOM " : "");
	fprintf(file, "%s\t\"%s\"\n", indent, m_name.c_str());
	fprintf(file, "%s\tmass %.6f\n", indent, m_mass.ToDouble());
	fprintf(file, "%s\torbit a=%.6f, e=%.6f, phase=%.6f\n", indent, m_orbit.GetSemiMajorAxis(), m_orbit.GetEccentricity(),
		m_orbit.GetOrbitalPhaseAtStart());
	fprintf(file, "%s\torbit a=%.6f, e=%.6f, orbMin=%.6f, orbMax=%.6f\n", indent, m_semiMajorAxis.ToDouble(), m_eccentricity.ToDouble(),
		m_orbMin.ToDouble(), m_orbMax.ToDouble());
	fprintf(file, "%s\t\toffset=%.6f, phase=%.6f, inclination=%.6f\n", indent, m_orbitalOffset.ToDouble(), m_orbitalPhaseAtStart.ToDouble(),
		m_inclination.ToDouble());
	if (m_type != TYPE_GRAVPOINT) {
		fprintf(file, "%s\tseed %u\n", indent, m_seed);
		fprintf(file, "%s\tradius %.6f, aspect %.6f\n", indent, m_radius.ToDouble(), m_aspectRatio.ToDouble());
		fprintf(file, "%s\taxial tilt %.6f, period %.6f, phase %.6f\n", indent, m_axialTilt.ToDouble(), m_rotationPeriod.ToDouble(),
			m_rotationalPhaseAtStart.ToDouble());
		fprintf(file, "%s\ttemperature %d\n", indent, m_averageTemp);
		fprintf(file, "%s\tmetalicity %.2f, volcanicity %.2f\n", indent, m_metallicity.ToDouble() * 100.0, m_volcanicity.ToDouble() * 100.0);
		fprintf(file, "%s\tvolatiles gas=%.2f, liquid=%.2f, ice=%.2f\n", indent, m_volatileGas.ToDouble() * 100.0,
			m_volatileLiquid.ToDouble() * 100.0, m_volatileIces.ToDouble() * 100.0);
		fprintf(file, "%s\tlife %.2f\n", indent, m_life.ToDouble() * 100.0);
		fprintf(file, "%s\tatmosphere oxidizing=%.2f, color=(%hhu,%hhu,%hhu,%hhu), density=%.6f\n", indent,
			m_atmosOxidizing.ToDouble() * 100.0, m_atmosColor.r, m_atmosColor.g, m_atmosColor.b, m_atmosColor.a, m_atmosDensity);
		fprintf(file, "%s\trings minRadius=%.2f, maxRadius=%.2f, color=(%hhu,%hhu,%hhu,%hhu)\n", indent, m_rings.minRadius.ToDouble() * 100.0,
			m_rings.maxRadius.ToDouble() * 100.0, m_rings.baseColor.r, m_rings.baseColor.g, m_rings.baseColor.b, m_rings.baseColor.a);
		fprintf(file, "%s\thuman activity %.2f, population %.0f, agricultural %.2f\n", indent, m_humanActivity.ToDouble() * 100.0,
			m_population.ToDouble() * 1e9, m_agricultural.ToDouble() * 100.0);
		if (!m_heightMapFilename.empty()) {
			fprintf(file, "%s\theightmap \"%s\", fractal %u\n", indent, m_heightMapFilename.c_str(), m_heightMapFractal);
		}
	}
	for (const SystemBody* kid : m_children) {
		assert(kid->m_parent == this);
		char buf[32];
		snprintf(buf, sizeof(buf), "%s\t", indent);
		kid->Dump(file, buf);
	}
	fprintf(file, "%s}\n", indent);
}

void SystemBody::ClearParentAndChildPointers()
{
	PROFILE_SCOPED()
	for (std::vector<SystemBody*>::iterator i = m_children.begin(); i != m_children.end(); ++i)
		(*i)->ClearParentAndChildPointers();
	m_parent = 0;
	m_children.clear();
}

StarSystem::~StarSystem()
{
	PROFILE_SCOPED()
	// clear parent and children pointers. someone (Lua) might still have a
	// reference to things that are about to be deleted
	m_rootBody->ClearParentAndChildPointers();
	if (m_cache)
		m_cache->RemoveFromAttic(m_path);
}

void StarSystem::Serialize(Serializer::Writer &wr, StarSystem *s)
{
	if (s) {
		wr.Byte(1);
		wr.Int32(s->m_path.sectorX);
		wr.Int32(s->m_path.sectorY);
		wr.Int32(s->m_path.sectorZ);
		wr.Int32(s->m_path.systemIndex);
	} else {
		wr.Byte(0);
	}
}

RefCountedPtr<StarSystem> StarSystem::Unserialize(Serializer::Reader &rd)
{
	if (rd.Byte()) {
		int sec_x = rd.Int32();
		int sec_y = rd.Int32();
		int sec_z = rd.Int32();
		int sys_idx = rd.Int32();
		return Pi::GetGalaxy()->GetStarSystem(SystemPath(sec_x, sec_y, sec_z, sys_idx));
	} else {
		return RefCountedPtr<StarSystem>(0);
	}
}

std::string StarSystem::ExportBodyToLua(FILE *f, SystemBody *body) {
	const int multiplier = 10000;
	int i;

	std::string code_name = body->GetName();
	std::transform(code_name.begin(), code_name.end(), code_name.begin(), ::tolower);
	code_name.erase(remove_if(code_name.begin(), code_name.end(), isspace), code_name.end());
	for(unsigned int j = 0; j < code_name.length(); j++) {
		if(code_name[j] == ',')
			code_name[j] = 'X';
		if(!((code_name[j] >= 'a' && code_name[j] <= 'z') ||
				(code_name[j] >= 'A' && code_name[j] <= 'Z') ||
				(code_name[j] >= '0' && code_name[j] <= '9')))
			code_name[j] = 'Y';
	}

	std::string code_list = code_name;

	for(i = 0; ENUM_BodyType[i].name != 0; i++) {
		if(ENUM_BodyType[i].value == body->GetType())
			break;
	}

	if(body->GetType() == SystemBody::TYPE_STARPORT_SURFACE) {
		fprintf(f,
			"local %s = CustomSystemBody:new(\"%s\", '%s')\n"
				"\t:latitude(math.deg2rad(%.1f))\n"
                "\t:longitude(math.deg2rad(%.1f))\n",

				code_name.c_str(),
				body->GetName().c_str(), ENUM_BodyType[i].name,
				body->m_inclination.ToDouble()*180/M_PI,
				body->m_orbitalOffset.ToDouble()*180/M_PI
				);
	} else {

		fprintf(f,
				"local %s = CustomSystemBody:new(\"%s\", '%s')\n"
				"\t:radius(f(%d,%d))\n"
				"\t:mass(f(%d,%d))\n",
				code_name.c_str(),
				body->GetName().c_str(), ENUM_BodyType[i].name,
				int(round(body->GetRadiusAsFixed().ToDouble()*multiplier)), multiplier,
				int(round(body->GetMassAsFixed().ToDouble()*multiplier)), multiplier
		);

		if(body->GetType() != SystemBody::TYPE_GRAVPOINT)
		fprintf(f,
				"\t:seed(%u)\n"
				"\t:temp(%d)\n"
				"\t:semi_major_axis(f(%d,%d))\n"
				"\t:eccentricity(f(%d,%d))\n"
				"\t:rotation_period(f(%d,%d))\n"
				"\t:axial_tilt(fixed.deg2rad(f(%d,%d)))\n"
				"\t:rotational_phase_at_start(fixed.deg2rad(f(%d,%d)))\n"
				"\t:orbital_phase_at_start(fixed.deg2rad(f(%d,%d)))\n"
				"\t:orbital_offset(fixed.deg2rad(f(%d,%d)))\n",
			body->GetSeed(), body->GetAverageTemp(),
			int(round(body->GetOrbit().GetSemiMajorAxis()/AU*multiplier)), multiplier,
			int(round(body->GetOrbit().GetEccentricity()*multiplier)), multiplier,
			int(round(body->m_rotationPeriod.ToDouble()*multiplier)), multiplier,
			int(round(body->GetAxialTilt()*multiplier)), multiplier,
			int(round(body->m_rotationalPhaseAtStart.ToDouble()*multiplier*180/M_PI)), multiplier,
			int(round(body->m_orbitalPhaseAtStart.ToDouble()*multiplier*180/M_PI)), multiplier,
			int(round(body->m_orbitalOffset.ToDouble()*multiplier*180/M_PI)), multiplier
		);

		if(body->GetType() == SystemBody::TYPE_PLANET_TERRESTRIAL)
			fprintf(f,
					"\t:metallicity(f(%d,%d))\n"
					"\t:volcanicity(f(%d,%d))\n"
					"\t:atmos_density(f(%d,%d))\n"
					"\t:atmos_oxidizing(f(%d,%d))\n"
					"\t:ocean_cover(f(%d,%d))\n"
					"\t:ice_cover(f(%d,%d))\n"
					"\t:life(f(%d,%d))\n",
				int(round(body->GetMetallicity()*multiplier)), multiplier,
				int(round(body->GetVolcanicity()*multiplier)), multiplier,
				int(round(body->GetVolatileGas()*multiplier)), multiplier,
				int(round(body->GetAtmosOxidizing()*multiplier)), multiplier,
				int(round(body->GetVolatileLiquid()*multiplier)), multiplier,
				int(round(body->GetVolatileIces()*multiplier)), multiplier,
				int(round(body->GetLife()*multiplier)), multiplier
			);
	}

	fprintf(f, "\n");

	if(body->m_children.size() > 0) {
		code_list = code_list + ", \n\t{\n";
		for (Uint32 ii = 0; ii < body->m_children.size(); ii++) {
			code_list = code_list + "\t" + ExportBodyToLua(f, body->m_children[ii]) + ", \n";
		}
		code_list = code_list + "\t}";
	}

	return code_list;

}

std::string StarSystem::GetStarTypes(SystemBody *body) {
	int i = 0;
	std::string types = "";

	if(body->GetSuperType() == SystemBody::SUPERTYPE_STAR) {
		for(i = 0; ENUM_BodyType[i].name != 0; i++) {
			if(ENUM_BodyType[i].value == body->GetType())
				break;
		}

		types = types + "'" + ENUM_BodyType[i].name + "', ";
	}

	for (Uint32 ii = 0; ii < body->m_children.size(); ii++) {
		types = types + GetStarTypes(body->m_children[ii]);
	}

	return types;
}

void StarSystem::ExportToLua(const char *filename) {
	FILE *f = fopen(filename,"w");
	int j;

	if(f == 0)
		return;

	fprintf(f,"-- Copyright © 2008-2012 Pioneer Developers. See AUTHORS.txt for details\n");
	fprintf(f,"-- Licensed under the terms of the GPL v3. See licenses/GPL-3.txt\n\n");

	std::string stars_in_system = GetStarTypes(m_rootBody.Get());

	for(j = 0; ENUM_PolitGovType[j].name != 0; j++) {
		if(ENUM_PolitGovType[j].value == GetSysPolit().govType)
			break;
	}

	fprintf(f,"local system = CustomSystem:new('%s', { %s })\n\t:govtype('%s')\n\t:short_desc('%s')\n\t:long_desc([[%s]])\n\n",
			GetName().c_str(), stars_in_system.c_str(), ENUM_PolitGovType[j].name, GetShortDescription().c_str(), GetLongDescription().c_str());

	fprintf(f, "system:bodies(%s)\n\n", ExportBodyToLua(f, m_rootBody.Get()).c_str());

	RefCountedPtr<const Sector> sec = Pi::GetGalaxy()->GetSector(GetPath());
	SystemPath pa = GetPath();

	fprintf(f, "system:add_to_sector(%d,%d,%d,v(%.4f,%.4f,%.4f))\n",
			pa.sectorX, pa.sectorY, pa.sectorZ,
			sec->m_systems[pa.systemIndex].GetPosition().x/Sector::SIZE,
			sec->m_systems[pa.systemIndex].GetPosition().y/Sector::SIZE,
			sec->m_systems[pa.systemIndex].GetPosition().z/Sector::SIZE);

	fclose(f);
}

void StarSystem::Dump(FILE* file, const char* indent, bool suppressSectorData) const
{
	if (suppressSectorData) {
		fprintf(file, "%sStarSystem {%s\n", indent, m_hasCustomBodies ? " CUSTOM-ONLY" : m_isCustom ? " CUSTOM" : "");
	} else {
		fprintf(file, "%sStarSystem(%d,%d,%d,%u) {\n", indent, m_path.sectorX, m_path.sectorY, m_path.sectorZ, m_path.systemIndex);
		fprintf(file, "%s\t\"%s\"\n", indent, m_name.c_str());
		fprintf(file, "%s\t%sEXPLORED%s\n", indent, m_unexplored ? "UN" : "", m_hasCustomBodies ? ", CUSTOM-ONLY" : m_isCustom ? ", CUSTOM" : "");
		fprintf(file, "%s\tfaction %s%s%s\n", indent, m_faction ? "\"" : "NONE", m_faction ? m_faction->name.c_str() : "", m_faction ? "\"" : "");
		fprintf(file, "%s\tseed %u\n", indent, static_cast<Uint32>(m_seed));
		fprintf(file, "%s\t%u stars%s\n", indent, m_numStars, m_numStars > 0 ? " {" : "");
		assert(m_numStars == m_stars.size());
		for (unsigned i = 0; i < m_numStars; ++i)
			fprintf(file, "%s\t\t%s\n", indent, EnumStrings::GetString("BodyType", m_stars[i]->GetType()));
		if (m_numStars > 0) fprintf(file, "%s\t}\n", indent);
	}
	fprintf(file, "%s\t%zu bodies, %zu spaceports \n", indent, m_bodies.size(), m_spaceStations.size());
	fprintf(file, "%s\tpopulation %.0f\n", indent, m_totalPop.ToDouble() * 1e9);
	fprintf(file, "%s\tgovernment %s/%s, lawlessness %.2f\n", indent, m_polit.GetGovernmentDesc(), m_polit.GetEconomicDesc(),
		m_polit.lawlessness.ToDouble() * 100.0);
	fprintf(file, "%s\teconomy type%s%s%s\n", indent, m_econType == 0 ? " NONE" : m_econType & GalacticEconomy::ECON_AGRICULTURE ? " AGRICULTURE" : "",
		m_econType & GalacticEconomy::ECON_INDUSTRY ? " INDUSTRY" : "", m_econType & GalacticEconomy::ECON_MINING ? " MINING" : "");
	fprintf(file, "%s\thumanProx %.2f\n", indent, m_humanProx.ToDouble() * 100.0);
	fprintf(file, "%s\tmetallicity %.2f, industrial %.2f, agricultural %.2f\n", indent, m_metallicity.ToDouble() * 100.0,
		m_industrial.ToDouble() * 100.0, m_agricultural.ToDouble() * 100.0);
	fprintf(file, "%s\ttrade levels {\n", indent);
	for (int i = 1; i < GalacticEconomy::COMMODITY_COUNT; ++i) {
		fprintf(file, "%s\t\t%s = %d\n", indent, EnumStrings::GetString("CommodityType", i), m_tradeLevel[i]);
	}
	fprintf(file, "%s\t}\n", indent);
	if (m_rootBody) {
		char buf[32];
		snprintf(buf, sizeof(buf), "%s\t", indent);
		assert(m_rootBody->GetPath().IsSameSystem(m_path));
		m_rootBody->Dump(file, buf);
	}
	fprintf(file, "%s}\n", indent);
}
