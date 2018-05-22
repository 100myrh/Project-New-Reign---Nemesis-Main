#ifndef FURNITURE_H_
#define FURNITURE_H_

#include <iostream>
#include <string>
#include <vector>
#include <locale>
#include <sstream>
#include <unordered_map>
#include <boost\regex.hpp>
#include <boost\algorithm\string.hpp>
// #include "tinyexpress.h"
#include "optionlist.h"
#include "registeranimation.h"
#include "animationinfo.h"
#include "animationutility.h"
#include "alphanum.hpp"

typedef std::vector<std::string> vecstr;
typedef std::unordered_map<std::string, std::string> SSMap;
typedef std::unordered_map<std::string, SSMap> ImportContainer;

struct MDException {};

class Furniture
{
private:
	std::string filepath;
	std::string filename;
	std::string format;
	std::string strID;
	vecstr clipname;
	vecstr eventID;
	vecstr variableID;
	SSMap IDExist;
	SSMap subFunctionIDs;
	SSMap mixOptRegis;
	ImportContainer* newImport;
	ImportContainer addition;
	bool isLastOrder = false;
	bool hasDuration = false;
	bool isEnd = false;
	bool known;
	int furnitureCount = 0;
	int* nextFunctionID;
	int order = 0;
	int lastOrder = 0;
	double duration = 0;
	std::vector<int>* lastState;
	std::unordered_map<int, std::string> AnimObject;
	std::unordered_map<std::string, bool> optionPicked;
	std::unordered_map<std::string, int> optionPickedCount;
	std::unordered_map<std::string, vecstr> addOn;
	std::unordered_map<std::string, vecstr> linkedOption;
	std::unordered_map<std::string, vecstr> mixOptRever;
	std::unordered_map<std::string, vecstr> furniturelines;
	std::unordered_map<std::string, std::map<std::string, vecstr, alphanum_less<std::string>>> asdlines;
	std::unordered_map<std::string, std::unordered_map<std::string, vecstr>> animdatalines;
	std::unordered_map<std::string, std::unordered_map<std::string, vecstr>> groupAddition;
	std::vector<std::shared_ptr<animationInfo>> groupAnimInfo;

	void multiChoice(std::string& line, vecstr& storeline, std::vector<std::unordered_map<std::string, bool>> groupOptionPicked, int numline, animationutility utility);
	bool newCondition(std::string condition, vecstr& storeline, std::vector<std::unordered_map<std::string, bool>> groupOptionPicked, int numline, animationutility utility);
	bool specialCondition(std::string condition, vecstr& storeline, int linecount, animationutility utility);
	bool GetFirstCondition(std::string firstCondition, vecstr optionInfo, int numline, std::vector<std::unordered_map<std::string, bool>> groupOptionPicked, bool isNot);
	bool GetSecondCondition(std::string secondCondition, int numline, std::vector<std::unordered_map<std::string, bool>> groupOptionPicked);
	void stateReplacer(std::string& line, std::string statenum, int stateID, int linecount, bool hasGroup, std::string otherAnimOrder = "", bool otherAnim = false);
	void processing(std::string& newline, vecstr& storeline, std::string masterFormat, int linecount, id eventid, id variableid, std::vector<int> stateID, int stateCountMultiplier, bool hasGroup, int optionMulti = -1, int animMulti = -1, std::string multiOption = "");
	void AnimDataLineProcess(vecstr originallines, vecstr& newlines, std::string format, std::vector<std::unordered_map<std::string, bool>> groupOptionPicked, int ASD = 0);

public:
	Furniture(std::unordered_map<std::string, vecstr> furnitureformat, std::string formatname, int furniturecount, std::string filepath, animationInfo& behaviorOption);
	std::string GetFilePath();
	vecstr GetFurnitureLine(std::string behaviorFile, int& ID, ImportContainer& import, id eventid, id variableid, std::vector<int>& stateID, int stateCountMultiplier, bool hasGroup, bool isCore);
	vecstr GetEventID();
	vecstr GetVariableID();
	SSMap GetNewIDs();
	SSMap GetMixOpt();
	ImportContainer GetAddition();
	std::vector<std::shared_ptr<animationInfo>> GetGroupAnimInfo();
	std::unordered_map<std::string, std::unordered_map<std::string, vecstr>> GetGroupAddition();
	inline void newID();
	void GetAnimSetData(std::unordered_map<std::string, std::map<std::string, vecstr, alphanum_less<std::string>>>& newASDLines);
	void GetAnimData(std::unordered_map<std::string, std::unordered_map<std::string, vecstr>>& newAnimDataLines);
	void existingASDProcess(vecstr ASDLines, std::map<int, vecstr>& extract, int ASD = 0);
	void addAnimData(std::unordered_map<std::string, std::unordered_map<std::string, vecstr>> animdata);
	void addAnimSetData(std::unordered_map<std::string, std::map<std::string, vecstr, alphanum_less<std::string>>> animsetdata);
	void storeAnimObject(vecstr animobjects, std::string listFilename, int lineCount);
	void addGroupAnimInfo(std::vector<std::shared_ptr<animationInfo>> animInfo);
	void setZeroEvent(std::string eventname);
	void setLastOrder(int order);
	void setOrder(int curOrder);
	int getOrder();
	bool isLast();
	bool isKnown();
	std::string addOnConverter(std::string curline, vecstr elements);

	std::string mainAnimEvent;
};

template <class T>
inline int numDigits(T number);
inline unsigned int CRC32Convert(std::string line);
inline bool isPassed(int condition, std::unordered_map<int, bool> IsConditionOpened);
inline bool clearGroupNum(std::string option2, std::unordered_map<std::string, bool> optionPicked, bool isNot, std::unordered_map<std::string, bool> groupOption);
std::string getOption(std::string curline, bool multi);
extern void motionDataReplacer(std::string& change, std::string format, std::string filename, int linecount, std::vector<std::shared_ptr<animationInfo>>& groupAnimInfo, int order, int lastOrder, bool isLastOrder, int animMulti, bool& isChange);
extern void rotationDataReplacer(std::string& change, std::string format, std::string filename, int linecount, std::vector<std::shared_ptr<animationInfo>>& groupAnimInfo, int order, int lastOrder, bool isLastOrder, int animMulti, bool& isChange);
extern void CRC32Replacer(std::string& line, std::string format, int linecount);
extern void addOnReplacer(std::string& line, std::unordered_map<std::string, vecstr> addOn, ImportContainer addition, std::unordered_map<std::string, std::unordered_map<std::string, vecstr>> groupAddition, SSMap mixOpt, unsigned int count, std::string format, int numline);
extern void animObjectReplacer(std::string& line, std::unordered_map<int, std::string> AnimObject, int id, std::string format, int linecount, bool otherAnim = false, std::string animNum = "");
extern void eventIDReplacer(std::string& line, std::string format, id eventid, std::string firstEvent, int linecount);
extern void variableIDReplacer(std::string& line, std::string format, id variableid, int linecount);
extern vecstr GetOptionInfo(std::string line, std::string templatename, int numline, size_t lastOrder, std::vector<std::shared_ptr<animationInfo>> groupAnimInfo, bool allowNoFixAnim, bool isCondition, bool isGroup = true, int animMulti = -1, int optionMulti = -1);

#endif