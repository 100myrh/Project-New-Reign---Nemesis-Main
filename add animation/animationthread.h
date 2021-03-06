#ifndef ANIMATIONTHREAD_H_
#define ANIMATIONTHREAD_H_

#include <memory>
#include <atomic>
#include "animationinfo.h"
#include "animationutility.h"

class Furniture;
struct groupTemplate;
struct single;
struct group;
struct master;

struct newAnimLock
{
private:
	std::atomic_flag coreLock = ATOMIC_FLAG_INIT;
	std::atomic_flag subIDLock = ATOMIC_FLAG_INIT;
	std::atomic_flag exportLock = ATOMIC_FLAG_INIT;

public:
	void lockCore();
	void releaseCore();

	void lockID();
	void releaseID();

	void lockExport();
	void releaseExport();
};

struct newAnimArgs
{
public:
	std::string modID, lowerBehaviorFile, ZeroEvent, ZeroVariable;
	int lastID;
	bool core, ignoreGroup, hasGroup;
	bool skip = false;
	bool& isCoreDone;
	std::shared_ptr<Furniture> dummyAnimation;
	std::unordered_map<int, int>& functionState;
	ImportContainer& exportID;
	id& eventid, variableid;
	newAnimLock& atomicLock;
	std::vector<int> stateID, stateMultiplier;
	std::shared_ptr<single> singleFunctionIDs;
	std::shared_ptr<group> subFunctionIDs;
	std::shared_ptr<master> groupFunctionIDs;
	std::vector<std::vector<std::shared_ptr<animationInfo>>>& groupAnimInfo;
	std::shared_ptr<vecstr> allEditLines;

	newAnimArgs(std::string n_modID, std::string n_lowerBehaviorFile, int n_lastID, bool n_core, std::shared_ptr<Furniture> n_newAnimation,
		std::shared_ptr<vecstr> n_allEditLines, bool& n_isCoreDone, std::unordered_map<int, int>& n_functionState, ImportContainer& n_exportID, id& n_eventid,
		id& n_variableid, newAnimLock& n_atomicLock, std::string n_ZeroEvent, std::string n_ZeroVariable, bool n_hasGroup, std::vector<int> n_stateID,
		std::vector<int> n_stateMultiplier, std::shared_ptr<single> n_singleFunctionIDs, std::shared_ptr<group> n_subFunctionIDs,
		std::shared_ptr<master> n_groupFunctionIDs, bool n_ignoreGroup, std::vector<std::vector<std::shared_ptr<animationInfo>>>& n_groupAnimInfo)
		: isCoreDone(n_isCoreDone), functionState(n_functionState),
		exportID(n_exportID), eventid(n_eventid), variableid(n_variableid), atomicLock(n_atomicLock), groupAnimInfo(n_groupAnimInfo)
	{
		allEditLines = n_allEditLines;
		modID = n_modID;
		lowerBehaviorFile = n_lowerBehaviorFile;
		core = n_core;
		ZeroEvent = n_ZeroEvent;
		ZeroVariable = n_ZeroVariable;
		ignoreGroup = n_ignoreGroup;
		hasGroup = n_hasGroup;
		stateID = n_stateID;
		stateMultiplier = n_stateMultiplier;
		lastID = n_lastID;
		singleFunctionIDs = n_singleFunctionIDs;
		subFunctionIDs = n_subFunctionIDs;
		groupFunctionIDs = n_groupFunctionIDs;
		dummyAnimation = n_newAnimation;
	}
};

struct newGroupArgs
{
public:
	std::string templateCode, lowerBehaviorFile, ZeroEvent, ZeroVariable, filename;
	int lastID, groupCount;
	std::vector<int> stateID;
	std::shared_ptr<groupTemplate> groupTemp;
	ImportContainer& exportID;
	id& eventid, variableid;
	newAnimLock& atomicLock;
	std::shared_ptr<master> groupFunctionIDs;
	std::vector<std::vector<std::shared_ptr<animationInfo>>>& groupAnimInfo;
	std::shared_ptr<vecstr> allEditLines;

	newGroupArgs(std::string n_templateCode, std::string n_lowerBehaviorFile, std::string n_filename, int n_lastID, int n_groupCount, std::vector<int> n_stateID,
		std::shared_ptr<groupTemplate> n_groupTemp, std::shared_ptr<vecstr> n_allEditLines, ImportContainer& n_exportID, id& n_eventid, id& n_variableid,
		newAnimLock& n_atomicLock, std::string n_ZeroEvent, std::string n_ZeroVariable, std::shared_ptr<master> n_groupFunctionIDs,
		std::vector<std::vector<std::shared_ptr<animationInfo>>>& n_groupAnimInfo)
		: exportID(n_exportID), eventid(n_eventid), variableid(n_variableid), atomicLock(n_atomicLock), groupAnimInfo(n_groupAnimInfo)
	{
		templateCode = n_templateCode;
		lowerBehaviorFile = n_lowerBehaviorFile;
		ZeroEvent = n_ZeroEvent;
		ZeroVariable = n_ZeroVariable;
		filename = n_filename;
		lastID = n_lastID;
		groupCount = n_groupCount;
		stateID = n_stateID;
		groupTemp = n_groupTemp;
		groupFunctionIDs = n_groupFunctionIDs;
		allEditLines = n_allEditLines;
	}
};

#endif
