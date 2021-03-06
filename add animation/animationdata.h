#ifndef ANIMATIONDATA_H_
#define ANIMATIONDATA_H_

#include "generator_utility.h"
#include "animationutility.h"
#include "add animation\furniture.h"
#include "add animation\animationsetdata.h"
#include "functions\writetextfile.h"

typedef std::unordered_map<std::string, SSMap> ImportContainer;

struct AnimDataFunct
{
	bool isNew = false;
	bool isCondition = false;
	bool isConditionOri = false;
	bool nextCondition = false;
	bool skip = false;

	int conditionOpen = false;
};

namespace AnimDataFormat
{
	enum position
	{
		totallinecount,
		behaviorfilecount,
		behaviorfilelist,
		animationname,
		uniquecode,
		eventnamecount,
		eventnamelist,
		totalcliplength,
		motiondatacount,
		motiondatalist,
		rotationdatacount,
		rotationdatalist,
		unknown2,
		unknown3,
		unknown4,
		unknown5,
		space,
		xerror,
		null
	};
}

struct AnimDataPack
{
	// anim data
	std::string name;
	std::string uniquecode;
	std::string unknown1;
	std::string unknown2;
	std::string unknown3;
	vecstr eventname;
};

struct InfoDataPack
{
	std::string uniquecode;
	std::string duration;
	vecstr motiondata;
	vecstr rotationdata;
};

struct AnimDataProject
{
	std::string projectActive;
	vecstr behaviorlist;
	std::string childActive;

	std::vector<AnimDataPack> animdatalist;
	std::vector<InfoDataPack> infodatalist;

	AnimDataProject() {}
	AnimDataProject(vecstr animdatafile, std::string filename, std::string filepath, std::string modcode = "nemesis");
	int GetAnimTotalLine();
	int GetInfoTotalLine();
};

struct MasterAnimData
{
	std::unordered_map<std::string, std::unordered_map<std::string, vecstr>> newAnimData;		// character, unique code, vector<string>; memory to access each node
	vecstr animDataChar;											// order of the character
	std::unordered_map<std::string, vecstr> animDataHeader;			// order of the character's header
	std::unordered_map<std::string, vecstr> animDataInfo;			// order of the character's info data
};

AnimDataFormat::position AnimDataPosition(vecstr animData, std::string character, std::string header, std::string modcode, std::string filepath, int linecount, int type, bool muteError = false);
AnimDataFormat::position AnimDataConvert(int type, int position, bool muteError);

#endif