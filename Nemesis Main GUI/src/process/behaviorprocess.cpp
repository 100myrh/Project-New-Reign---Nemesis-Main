#include <QThread>

#include <QtCore\QCoreApplication.h>

#include <QtConcurrent\qtconcurrentrun.h>

#include <boost\thread.hpp>
#include <boost\asio\post.hpp>
#include <boost\asio\thread_pool.hpp>

#include "version.h"
#include "addanims.h"
#include "filechecker.h"
#include "behaviorcheck.h"
#include "installscripts.h"
#include "behaviorprocess.h"
#include "generator_utility.h"
#include "behaviorgenerator.h"
#include "behaviorsubprocess.h"

#include "functions\renew.h"

#include "utilities\Terminator.h"
#include "utilities\externalscript.h"

#include "add animation\singletemplate.h"
#include "add animation\playerexclusive.h"
#include "add animation\registeranimation.h"

#include "MessageHandler.h"


using namespace std;

extern bool processdone;
extern mutex processlock;
extern mutex cv2_m;
extern condition_variable cv2;
extern condition_variable cv;
extern Terminator* p_terminate;
extern vecstr failedBehaviors;
extern atomic<int> m_RunningThread;
extern atomic<int> behaviorRun;
extern atomic<int> extraCore;

mutex anim_lock;
boost::atomic_flag atomic_lock = BOOST_ATOMIC_FLAG_INIT;

extern vecstr fileCheckMsg;

void addOnInstall(string templine, string& elementLine, vector<vecstr>& groupAddOnElement, ImportContainer& addOn,
	unordered_map<string, unordered_map<string, vecstr>>& groupAddOn);

BehaviorStart::BehaviorStart()
{
}

BehaviorStart::~BehaviorStart()
{
	if (!cmdline && error) error = false;
}

void BehaviorStart::addBehaviorPick(NemesisMainGUI* newWidget, vecstr behaviorOrder, unordered_map<string, bool> behaviorPick)
{
	behaviorPriority = behaviorOrder;
	chosenBehavior = behaviorPick;
	behaviorProcess.newWidget(newWidget);
}

void BehaviorStart::addBehaviorPick(vecstr behaviorOrder, unordered_map<string, bool> behaviorPick)
{
	behaviorPriority = behaviorOrder;
	chosenBehavior = behaviorPick;
	cmdline = true;
}

void BehaviorStart::message(string input)
{
	emit incomingMessage(QString::fromStdString(input));
}

void BehaviorStart::InitializeGeneration()
{
	std::thread* checkThread = nullptr;

	try
	{
		try
		{
			try
			{
				extraCore = 0;
				ClearGlobal();
				milestoneStart();
				string version;

				if (!isEngineUpdated(version))
				{
					interMsg(TextBoxMessage(1000));
					m_RunningThread = 0;
					unregisterProcess(true);
					emit disableLaunch();
					return;
				}

				interMsg(TextBoxMessage(1003));
				interMsg(TextBoxMessage(1017) + ": " + version);
				interMsg("");
				interMsg(TextBoxMessage(1004));
				interMsg("");

				if (error) throw nemesis::exception();

				// Check the existence of required files
				FileCheck();

				// Script Run
				RunScript("scripts\\launcher\\start\\");
				ClearGlobal();
				characterHKX();
				GetBehaviorPath();
				GetBehaviorProject();
				GetBehaviorProjectPath();
				behaviorActivateMod(behaviorPriority);
				ClearTempXml();

				if (error) throw nemesis::exception();

				GenerateBehavior(checkThread);
			}
			catch (exception& ex)
			{
				ErrorMessage(6002, "None", ex.what());
			}
		}
		catch (nemesis::exception&)
		{
			// resolved exception
		}
	}
	catch (...)
	{
		try
		{
			ErrorMessage(6002, "None", "Unknown");
		}
		catch (nemesis::exception&)
		{
			// resolved exception
		}
	}

	if (checkThread != nullptr)
	{
		try
		{
			if (checkThread->joinable()) checkThread->join();
		}
		catch (exception)
		{
		}

		delete checkThread;
	}

	EndAttempt();
}

void BehaviorStart::GenerateBehavior(std::thread*& checkThread)
{
	// register animation & organize AE n Var
	string directory = "temp_behaviors\\";
	unordered_map<string, int> animationCount;		// animation type counter; use to determine how many of the that type of animation have been installed
	shared_ptr<getTemplate> BehaviorTemplate = make_shared<getTemplate>();					// get animation type

	vector<unique_ptr<registerAnimation>> animationList = move(openFile(BehaviorTemplate.get()));	// get anim list installed by mods
	unordered_map<string, vector<shared_ptr<Furniture>>> newAnimation;						// record each of the animation created from registerAnimation

	mapSetString newAnimEvent;				// template code, list of events
	mapSetString newAnimVariable;			// template code, list of variables

	unordered_map<string, var> AnimVar;
	unordered_map<string, vector<string>> modAnimBehavior;				// behavior directory, list of behavior files; use to get behavior reference
	unordered_map<string, unordered_map<int, bool>> ignoreFunction;		// behavior file, function ID, true/false; is the function part of animation template?

	if (error) throw nemesis::exception();

	if (PCEACheck()) ReadPCEA();

	checkThread = new std::thread(checkAllFiles, nemesisInfo->GetDataPath() + "meshes\\actors");

	if (animReplaced.size() > 0 || animationList.size() > 0) interMsg("");

	DebugLogging("Registering new animations...");

	// read each animation list file'
	for (unsigned int i = 0; i < animationList.size(); ++i)
	{
		set<string> coreRegistered;		// list of registered core behavior
		string modID = animationList[i]->modID;

		// read each animation in a group of the same type
		for (auto it = animationList[i]->templateType.begin(); it != animationList[i]->templateType.end(); ++it)
		{
			if (error) throw nemesis::exception();

			string firstAE;
			string coreModName = "";
			string templatecode = it->first;
			DebugLogging("Registering " + templatecode + " animations");
			vector<shared_ptr<animationInfo>> animInfoGroup;
			shared_ptr<unordered_map<string, AnimTemplate>> animTemplate = make_shared<unordered_map<string, AnimTemplate>>();	// behavior file, template

			// project, header, template
			shared_ptr<unordered_map<string, unordered_map<string, AnimTemplate>>> animdataTemplate =
				make_shared<unordered_map<string, unordered_map<string, AnimTemplate>>>();

			// project, header, template
			shared_ptr<unordered_map<string, map<string, AnimTemplate, alphanum_less>>> asdTemplate =
				make_shared<unordered_map<string, map<string, AnimTemplate, alphanum_less>>>();

			unordered_map<string, vecstr>* functionlines = &BehaviorTemplate->behaviortemplate[templatecode];
			unordered_map<string, unordered_map<string, vecstr>>* animdatalines = &BehaviorTemplate->animdatatemplate[templatecode];
			unordered_map<string, map<string, vecstr, alphanum_less>>* asdlines = &BehaviorTemplate->asdtemplate[templatecode];
			newAnimation[templatecode].reserve(50 * memory);
			animationCount[templatecode]++;
			bool isGroup;
			bool ignoreGroup = BehaviorTemplate->optionlist[templatecode].ignoreGroup;

			// Create proxy new animation to link master behavior branch with sub-branch, which is used as a core
			if (BehaviorTemplate->coreTemplate[templatecode].length() > 0)
			{
				coreModName = "Nemesis_" + modID + "_";
				string corepath = string(behaviorPath[BehaviorTemplate->coreTemplate[templatecode]]);
				string corename = GetFileName(corepath);
				corepath = corepath.substr(0, corepath.length() - corename.length());
				DebugLogging("Core behavior name: " + corename);
				DebugLogging("Core behavior destination: " + corepath);

				if (!isFileExist(corepath + "nemesis_" + corename + ".txt") && !isFileExist(corepath + "nemesis_" + corename + ".xml"))
				{
					WarningMessage(1004, BehaviorTemplate->optionlist[templatecode].coreBehavior, templatecode, corepath + "nemesis_" + corename + ".txt");
				}
				else
				{
					string corecode = BehaviorTemplate->coreBehaviorCode[BehaviorTemplate->coreTemplate[templatecode]];

					if (coreRegistered.find(corecode) == coreRegistered.end())
					{
						DebugLogging("Core Registration: " + corecode);
						coreRegistered.insert(corecode);
						unordered_map<string, vecstr>* c_functionlines = &BehaviorTemplate->behaviortemplate[corecode];
						shared_ptr<animationInfo> dummy = make_shared<animationInfo>();

						if (isFileExist(corepath + "FNIS_" + modID + "_" + BehaviorTemplate->optionlist[corecode].coreBehavior + ".hkx"))
						{
							dummy->addFilename("FNIS_" + modID + "_" + BehaviorTemplate->optionlist[corecode].coreBehavior + ".hkx");
						}
						else
						{
							dummy->addFilename(coreModName + BehaviorTemplate->optionlist[corecode].coreBehavior + ".hkx");
							coreModList[nemesis::to_lower_copy(BehaviorTemplate->optionlist[corecode].coreBehavior) + ".txt"].push_back(coreModName);
						}

						dummy->ignoreGroup = false;
						dummy->optionPickedCount[corecode] = 1;
						animationCount[corecode]++;

						if (BehaviorTemplate->optionlist[corecode].core)
						{
							newAnimation[corecode].emplace_back(make_unique<Furniture>(make_shared<unordered_map<string, AnimTemplate>>(), corecode,
								*c_functionlines, animationCount[corecode], "", *dummy));
						}
						else
						{
							// behavior file, template
							shared_ptr<unordered_map<string, AnimTemplate>> c_animTemplate = make_shared<unordered_map<string, AnimTemplate>>();

							for (auto& func : *c_functionlines)
							{
								if ((*c_animTemplate)[func.first].size == 0)
								{
									(*c_animTemplate)[func.first].hasDuration = false;
									(*c_animTemplate)[func.first].ExamineTemplate(templatecode, func.first, func.second, false, false,
										BehaviorTemplate->optionlist[corecode]);
								}
							}

							newAnimation[corecode].emplace_back(make_unique<Furniture>(c_animTemplate, corecode, unordered_map<string, vecstr>(),
								animationCount[corecode], "", *dummy));
						}

						vector<shared_ptr<animationInfo>> animInfoGroup = { dummy };
						newAnimation[corecode].back()->addGroupAnimInfo(animInfoGroup);
						newAnimation[corecode].back()->setLastOrder(0);
					}
				}
			}

			if (BehaviorTemplate->behaviortemplate.find(templatecode + "_group") != BehaviorTemplate->behaviortemplate.end() &&
				BehaviorTemplate->behaviortemplate[templatecode + "_group"].size() != 0)
			{
				animInfoGroup.reserve(100);
				isGroup = true;
			}
			else
			{
				animInfoGroup.reserve(50 * memory);
				isGroup = false;
			}

			// Behavior
			for (auto& func : *functionlines)
			{
				if ((*animTemplate)[func.first].size == 0)
				{
					(*animTemplate)[func.first].ExamineTemplate(templatecode, func.first, func.second, false, false, BehaviorTemplate->optionlist[templatecode]);
				}
			}

			for (auto& proj : *animdatalines)
			{
				for (auto& header : proj.second)
				{
					if ((*animdataTemplate)[proj.first][header.first].size == 0)
					{
						(*animdataTemplate)[proj.first][header.first].ExamineTemplate(templatecode, "animationdatasinglefile.txt", header.second, false, false,
							BehaviorTemplate->optionlist[templatecode]);
					}
				}
			}

			for (auto& proj : *asdlines)
			{
				for (auto& header : proj.second)
				{
					if ((*asdTemplate)[proj.first][header.first].size == 0)
					{
						(*asdTemplate)[proj.first][header.first].ExamineTemplate(templatecode, "animationsetdatasinglefile.txt", header.second, false, false,
							BehaviorTemplate->optionlist[templatecode]);
					}
				}
			}

			// read each line and generate lines categorized using template code (animation type)
			try
			{
				for (int j = 0; j < it->second; ++j)
				{
					int order = animationList[i]->isMulti[templatecode][j];
					int lastOrder = animationList[i]->last[templatecode][j];

					newAnimation[templatecode].emplace_back(make_unique<Furniture>(animTemplate, templatecode, *functionlines,
						animationCount[templatecode], modID + "\\", *animationList[i]->animInfo[templatecode][j]));

					newAnimation[templatecode].back()->addAnimData(*animdataTemplate);
					newAnimation[templatecode].back()->addAnimSetData(*asdTemplate);

					newAnimation[templatecode].back()->coreModID = coreModName;
					vecstr tempEventID = newAnimation[templatecode].back()->GetEventID();
					vecstr tempVariableID = newAnimation[templatecode].back()->GetVariableID();
					newAnimEvent[templatecode + coreModName].insert(tempEventID.begin(), tempEventID.end());
					newAnimVariable[templatecode + coreModName].insert(tempVariableID.begin(), tempVariableID.end());

					if (order != 0)
					{
						newAnimation[templatecode].back()->setOrder(order);
					}
					else
					{
						firstAE = newAnimation[templatecode].back()->mainAnimEvent;

						if (BehaviorTemplate->optionlist[templatecode].eleEventGroupF.size() != 0)
						{
							vector<vecstr>* elementList = &BehaviorTemplate->optionlist[templatecode].eleEventGroupF;
							vector<string>* elementListLine = &BehaviorTemplate->optionlist[templatecode].eleEventGroupFLine;
							ImportContainer addOn = newAnimation[templatecode].back()->GetAddition();
							unordered_map<string, unordered_map<string, vecstr>> groupAddOn = newAnimation[templatecode].back()->GetGroupAddition();

							for (unsigned int k = 0; k < elementList->size(); ++k)
							{
								vecstr* element = &(*elementList)[k];
								string elementLine = (*elementListLine)[k];
								vector<vecstr> groupAddOnElement;
								groupAddOnElement.reserve(memory / 10);

								for (unsigned int l = 0; l < element->size(); ++l)
								{
									string templine = (*element)[l];

									if (nemesis::iequals(templine, "main_anim_event"))
									{
										templine = newAnimation[templatecode].back()->mainAnimEvent;
										elementLine.replace(elementLine.find("$$"), 2, templine);
									}
									else
									{
										addOnInstall(templine, elementLine, groupAddOnElement, addOn, groupAddOn);
									}
								}

								if (groupAddOnElement.size() != 0)
								{
									vecstr animEvent = newAnimationElement(elementLine, groupAddOnElement, 0);

									for (unsigned int l = 0; l < animEvent.size(); ++l)
									{
										newAnimEvent[templatecode + coreModName].insert(animEvent[l]);
									}
								}
								else
								{
									newAnimEvent[templatecode + coreModName].insert(elementLine);
								}
							}
						}

						if (BehaviorTemplate->optionlist[templatecode].eleVarGroupF.size() != 0)
						{
							vector<vecstr>* elementList = &BehaviorTemplate->optionlist[templatecode].eleVarGroupF;
							vector<string>* elementListLine = &BehaviorTemplate->optionlist[templatecode].eleVarGroupFLine;
							ImportContainer addOn = newAnimation[templatecode].back()->GetAddition();
							unordered_map<string, unordered_map<string, vecstr>> groupAddOn = newAnimation[templatecode].back()->GetGroupAddition();

							for (unsigned int k = 0; k < elementList->size(); ++k)
							{
								vecstr* element = &(*elementList)[k];
								string elementLine = (*elementListLine)[k];
								vector<vecstr> groupAddOnElement;
								groupAddOnElement.reserve(memory / 10);

								for (unsigned int l = 0; l < element->size(); ++l)
								{
									string templine = (*element)[l];
									bool isBreak = false;

									for (auto it = addOn.begin(); it != addOn.end(); ++it)
									{
										for (auto iter = it->second.begin(); iter != it->second.end(); ++iter)
										{
											if (nemesis::iequals(templine, it->first + "[" + iter->first + "]"))
											{
												elementLine.replace(elementLine.find("$$"), 2, iter->second);
												isBreak = true;
												break;
											}
										}

										if (isBreak) break;
									}

									if (!isBreak)
									{
										bool isDone = false;

										for (auto it = groupAddOn.begin(); it != groupAddOn.end(); ++it)
										{
											for (auto iter = it->second.begin(); iter != it->second.end(); ++iter)
											{
												if (nemesis::iequals(templine, it->first + "[" + iter->first + "]"))
												{
													elementLine.replace(elementLine.find("$$"), 2, "##");
													isDone = true;
													break;
												}

												groupAddOnElement.push_back(iter->second);
											}

											if (isDone) break;
										}
									}
								}

								if (groupAddOnElement.size() != 0)
								{
									vecstr animVar = newAnimationElement(elementLine, groupAddOnElement, 0);

									for (unsigned int l = 0; l < animVar.size(); ++l)
									{
										newAnimVariable[templatecode + coreModName].insert(animVar[l]);
									}
								}
								else
								{
									newAnimVariable[templatecode + coreModName].insert(elementLine);
								}
							}
						}
					}

					newAnimation[templatecode].back()->setLastOrder(lastOrder);
					animInfoGroup.push_back(move(animationList[i]->animInfo[templatecode][j]));

					if (!ignoreGroup) animationCount[templatecode]++;

					if (lastOrder == order)
					{
						size_t animationSize = newAnimation[templatecode].size();
						animInfoGroup.shrink_to_fit();

						for (int k = 0; k < order + 1; ++k)
						{
							newAnimation[templatecode][animationSize - k - 1]->addGroupAnimInfo(animInfoGroup);
						}

						animInfoGroup.clear();
						animInfoGroup.reserve(100);

						if (ignoreGroup)
						{
							animationCount[templatecode]++;
						}
						else
						{
							if (BehaviorTemplate->optionlist[templatecode].eleEventGroupL.size() != 0)
							{
								vector<vecstr>* elementList = &BehaviorTemplate->optionlist[templatecode].eleEventGroupL;
								vector<string>* elementListLine = &BehaviorTemplate->optionlist[templatecode].eleEventGroupLLine;
								ImportContainer addOn = newAnimation[templatecode].back()->GetAddition();
								unordered_map<string, unordered_map<string, vecstr>> groupAddOn = newAnimation[templatecode].back()->GetGroupAddition();

								for (unsigned int k = 0; k < elementList->size(); ++k)
								{
									vecstr* element = &(*elementList)[k];
									string elementLine = (*elementListLine)[k];
									vector<vecstr> groupAddOnElement;
									groupAddOnElement.reserve(memory / 10);

									for (unsigned int l = 0; l < element->size(); ++l)
									{
										string templine = (*element)[l];

										if (nemesis::iequals(templine, "main_anim_event"))
										{
											templine = newAnimation[templatecode].back()->mainAnimEvent;
											elementLine.replace(elementLine.find("$$"), 2, templine);
										}
										else
										{
											addOnInstall(templine, elementLine, groupAddOnElement, addOn, groupAddOn);
										}
									}

									if (groupAddOnElement.size() != 0)
									{
										vecstr animEvent = newAnimationElement(elementLine, groupAddOnElement, 0);

										for (unsigned int l = 0; l < animEvent.size(); ++l)
										{
											newAnimEvent[templatecode + coreModName].insert(animEvent[l]);
										}
									}
									else
									{
										newAnimEvent[templatecode + coreModName].insert(elementLine);
									}
								}
							}

							if (BehaviorTemplate->optionlist[templatecode].eleVarGroupL.size() != 0)
							{
								vector<vecstr>* elementList = &BehaviorTemplate->optionlist[templatecode].eleVarGroupL;
								vector<string>* elementListLine = &BehaviorTemplate->optionlist[templatecode].eleVarGroupLLine;
								ImportContainer addOn = newAnimation[templatecode].back()->GetAddition();
								unordered_map<string, unordered_map<string, vecstr>> groupAddOn = newAnimation[templatecode].back()->GetGroupAddition();

								for (unsigned int k = 0; k < elementList->size(); ++k)
								{
									vecstr* element = &(*elementList)[k];
									string elementLine = (*elementListLine)[k];
									vector<vecstr> groupAddOnElement;
									groupAddOnElement.reserve(memory / 10);

									for (unsigned int l = 0; l < element->size(); ++l)
									{
										string templine = (*element)[l];
										addOnInstall(templine, elementLine, groupAddOnElement, addOn, groupAddOn);
									}

									if (groupAddOnElement.size() != 0)
									{
										vecstr animVar = newAnimationElement(elementLine, groupAddOnElement, 0);

										for (unsigned int l = 0; l < animVar.size(); ++l)
										{
											newAnimVariable[templatecode + coreModName].insert(animVar[l]);
										}
									}
									else
									{
										newAnimVariable[templatecode + coreModName].insert(elementLine);
									}
								}
							}
						}
					}

					if (BehaviorTemplate->optionlist[templatecode].eleEvent.size() != 0)
					{
						vector<vecstr>* elementList = &BehaviorTemplate->optionlist[templatecode].eleEvent;
						vector<string>* elementListLine = &BehaviorTemplate->optionlist[templatecode].eleEventLine;
						ImportContainer addOn = newAnimation[templatecode].back()->GetAddition();
						unordered_map<string, unordered_map<string, vecstr>> groupAddOn = newAnimation[templatecode].back()->GetGroupAddition();

						for (unsigned int k = 0; k < elementList->size(); ++k)
						{
							vecstr* element = &(*elementList)[k];
							string elementLine = (*elementListLine)[k];
							vector<vecstr> groupAddOnElement;
							groupAddOnElement.reserve(memory / 10);

							for (unsigned int l = 0; l < element->size(); ++l)
							{
								string templine = (*element)[l];

								if (nemesis::iequals(templine, "main_anim_event"))
								{
									templine = newAnimation[templatecode].back()->mainAnimEvent;
									elementLine.replace(elementLine.find("$$"), 2, templine);
								}
								else
								{
									addOnInstall(templine, elementLine, groupAddOnElement, addOn, groupAddOn);
								}
							}

							if (groupAddOnElement.size() != 0)
							{
								vecstr animEvent = newAnimationElement(elementLine, groupAddOnElement, 0);

								for (unsigned int l = 0; l < animEvent.size(); ++l)
								{
									newAnimEvent[templatecode + coreModName].insert(animEvent[l]);
								}
							}
							else
							{
								newAnimEvent[templatecode + coreModName].insert(elementLine);
							}
						}
					}

					if (BehaviorTemplate->optionlist[templatecode].eleVar.size() != 0)
					{
						vector<vecstr>* elementList = &BehaviorTemplate->optionlist[templatecode].eleVar;
						vector<string>* elementListLine = &BehaviorTemplate->optionlist[templatecode].eleVarLine;
						ImportContainer addOn = newAnimation[templatecode].back()->GetAddition();
						unordered_map<string, unordered_map<string, vecstr>> groupAddOn = newAnimation[templatecode].back()->GetGroupAddition();

						for (unsigned int k = 0; k < elementList->size(); ++k)
						{
							vecstr* element = &(*elementList)[k];
							string elementLine = (*elementListLine)[k];
							vector<vecstr> groupAddOnElement;
							groupAddOnElement.reserve(memory / 10);

							for (unsigned int l = 0; l < element->size(); ++l)
							{
								string templine = (*element)[l];

								if (nemesis::iequals(templine, "main_anim_event"))
								{
									templine = newAnimation[templatecode].back()->mainAnimEvent;
									elementLine.replace(elementLine.find("$$"), 2, templine);
								}
								else
								{
									addOnInstall(templine, elementLine, groupAddOnElement, addOn, groupAddOn);
								}
							}

							if (groupAddOnElement.size() != 0)
							{
								vecstr animVar = newAnimationElement(elementLine, groupAddOnElement, 0);

								for (unsigned int l = 0; l < animVar.size(); ++l)
								{
									newAnimVariable[templatecode + coreModName].insert(animVar[l]);
								}
							}
							else
							{
								newAnimVariable[templatecode + coreModName].insert(elementLine);
							}
						}
					}

					for (unsigned int k = 0; k < tempVariableID.size(); ++k)
					{
						string name = tempVariableID[k];
						AnimVar[name] = animationList[i]->AnimVar[name];
					}
				}
			}
			catch (nemesis::exception)
			{
				ErrorMessage(6000);
			}

			newAnimation[templatecode].shrink_to_fit();

			if (error) throw nemesis::exception();
		}
	}

	if (error) throw nemesis::exception();

	{
		int multi = 0;

		for (auto& modlist : coreModList)
		{
			multi = int(modlist.second.size());
			--multi;
		}

		filenum += (multi * 10);
		DebugLogging("Process count: " + to_string(filenum));
		emit progressMax(filenum);
	}

	DebugLogging("New animations registration complete");
	emit progressUp();

	// Ignore function in master branch
	for (auto it = BehaviorTemplate->grouplist.begin(); it != BehaviorTemplate->grouplist.end(); ++it)
	{
		for (auto& templatecode : it->second)
		{
			// existing function
			for (unsigned int k = 0; k < BehaviorTemplate->existingFunctionID[templatecode][it->first].size(); ++k)
			{
				ignoreFunction[it->first + ".txt"][BehaviorTemplate->existingFunctionID[templatecode][it->first][k]] = true;
			}
		}
	}

	if (error) throw nemesis::exception();
	emit progressUp();

	if (pcealist.size() > 0 || alternateAnim.size() > 0)
	{
		InstallScripts* ScriptWorker = new InstallScripts;		// install PCEA & AA script
		connect(ScriptWorker, SIGNAL(end()), this, SLOT(EndAttempt()));
		connect(ScriptWorker, SIGNAL(end()), ScriptWorker, SLOT(deleteLater()));
		QtConcurrent::run(ScriptWorker, &InstallScripts::Run);
	}

	{
		string filename(nemesisInfo->GetDataPath() + "scripts\\FNIS.pex");
		boost::filesystem::copy_file(boost::filesystem::path("alternate animation\\FNBE.pex"), filename, boost::filesystem::copy_option::overwrite_if_exists);
		FILE* f;
		fopen_s(&f, filename.c_str(), "r+b");

		if (f)
		{
			std::fseek(f, 20, SEEK_SET);
			unsigned char charcode = 73;
			std::fwrite(&charcode, sizeof(charcode), 1, f);

			std::fseek(f, 21, SEEK_SET);
			charcode = 83;
			std::fwrite(&charcode, sizeof(charcode), 1, f);

			std::fseek(f, 45, SEEK_SET);
			charcode = 73;
			std::fwrite(&charcode, sizeof(charcode), 1, f);

			std::fseek(f, 46, SEEK_SET);
			charcode = 83;
			std::fwrite(&charcode, sizeof(charcode), 1, f);

			std::fseek(f, 495, SEEK_SET);
			charcode = 49;
			std::fwrite(&charcode, sizeof(charcode), 1, f);

			std::fclose(f);
		}
		else
		{
			ErrorMessage(3002, filename);
		}
	}

	if (error) throw nemesis::exception();

	vecstr filelist;
	read_directory(directory, filelist);
	emit progressUp();
	behaviorRun = 1;

	for (unsigned int i = 0; i < filelist.size(); ++i)
	{
		if (!boost::filesystem::is_directory(directory + filelist[i]))
		{
			string lowerFileName = nemesis::to_lower_copy(filelist[i]);

			if (coreModList.find(lowerFileName) != coreModList.end())
			{
				int repeat = int(coreModList[lowerFileName].size()) - 1;
				filenum += repeat * 10;
			}
		}
		else if (wordFind(filelist[i], "_1stperson") != NOT_FOUND)
		{
			vecstr fpfilelist;
			read_directory(directory + filelist[i], fpfilelist);

			for (unsigned int j = 0; j < fpfilelist.size(); ++j)
			{
				string lowerFileName = nemesis::to_lower_copy(filelist[i] + "\\" + fpfilelist[j]);

				if (coreModList.find(lowerFileName) != coreModList.end())
				{
					int repeat = int(coreModList[lowerFileName].size()) - 1;
					filenum += repeat * 10;
				}
			}
		}
	}

	boost::asio::thread_pool mt;
	vector<BehaviorSub*> behaviorSubList;

	try
	{
		for (unsigned int i = 0; i < filelist.size(); ++i)
		{
			if (error) throw nemesis::exception();

			if (!boost::filesystem::is_directory(directory + filelist[i]))
			{
				string modID = "";
				bool isCore = false;
				int repeatcount = 0;
				int repeat = 1;
				string lowerFileName = nemesis::to_lower_copy(filelist[i]);

				if (coreModList.find(lowerFileName) != coreModList.end())
				{
					repeat = int(coreModList[lowerFileName].size());
					isCore = true;
				}

				while (repeatcount < repeat)
				{
					if (error) throw nemesis::exception();

					if (isCore) modID = coreModList[lowerFileName][repeatcount];

					bool skip = false;
					string tempfilename = lowerFileName.substr(0, lowerFileName.find_last_of("."));
					string temppath = behaviorPath[tempfilename];

					if (temppath.length() != 0)
					{
						size_t nextpos = 0;
						size_t lastpos = temppath.find_last_of("\\");

						while (temppath.find("\\", nextpos) != lastpos)
						{
							nextpos = temppath.find("\\", nextpos) + 1;
						}

						temppath = temppath.substr(nextpos, lastpos - nextpos);
					}

					if (error) throw nemesis::exception();

					BehaviorSub* worker = new BehaviorSub;
					behaviorSubList.push_back(worker);

					worker->addInfo(directory, filelist, i, behaviorPriority, chosenBehavior, BehaviorTemplate, newAnimation, AnimVar, newAnimEvent, newAnimVariable,
						ignoreFunction, false, modID, this);

					if (!cmdline)
					{
						behaviorProcess.newProcess(worker);
						behaviorProcess.connect();
					}

					if (lowerFileName == "animationdatasinglefile.txt")
					{
						boost::asio::post(mt, boost::bind(&BehaviorSub::AnimDataCompilation, worker));
					}
					else if (lowerFileName == "animationsetdatasinglefile.txt")
					{
						boost::asio::post(mt, boost::bind(&BehaviorSub::ASDCompilation, worker));
					}
					else if (temppath.find("characters") != 0)
					{
						boost::asio::post(mt, boost::bind(&BehaviorSub::BehaviorCompilation, worker));
					}
					else
					{
						worker->isCharacter = true;
						boost::asio::post(mt, boost::bind(&BehaviorSub::BehaviorCompilation, worker));
					}

					++repeatcount;
				}
			}
			else if (wordFind(filelist[i], "_1stperson") != NOT_FOUND)
			{
				vecstr fpfilelist;
				read_directory(directory + filelist[i], fpfilelist);

				for (auto& curfile : fpfilelist)
				{
					curfile = filelist[i] + "\\" + curfile;
				}

				for (unsigned int j = 0; j < fpfilelist.size(); ++j)
				{
					if (error) throw nemesis::exception();

					if (!boost::filesystem::is_directory(directory + fpfilelist[j]))
					{
						string modID = "";
						bool isCore = false;
						int repeatcount = 0;
						int repeat = 1;
						string lowerFileName = nemesis::to_lower_copy(fpfilelist[j]);

						if (coreModList.find(lowerFileName) != coreModList.end())
						{
							repeat = int(coreModList[lowerFileName].size());
							isCore = true;

							if (repeat > 1) filenum += (repeat - 1) * 10;
						}

						while (repeatcount < repeat)
						{
							if (error) throw nemesis::exception();

							if (isCore) modID = coreModList[lowerFileName][repeatcount];

							bool skip = false;
							string tempfilename = lowerFileName.substr(0, lowerFileName.find_last_of("."));
							string temppath = behaviorPath[tempfilename];

							if (temppath.length() != 0)
							{
								size_t nextpos = 0;
								size_t lastpos = temppath.find_last_of("\\");

								while (temppath.find("\\", nextpos) != lastpos)
								{
									nextpos = temppath.find("\\", nextpos) + 1;
								}

								temppath = temppath.substr(nextpos, lastpos - nextpos);
							}

							BehaviorSub* worker = new BehaviorSub;
							behaviorSubList.push_back(worker);

							worker->addInfo(directory, fpfilelist, j, behaviorPriority, chosenBehavior, BehaviorTemplate, newAnimation, AnimVar, newAnimEvent,
								newAnimVariable, ignoreFunction, false, modID, this);

							if (!cmdline)
							{
								behaviorProcess.newProcess(worker);
								behaviorProcess.connect();
							}

							if (lowerFileName == "animationdatasinglefile.txt")
							{
								boost::asio::post(mt, boost::bind(&BehaviorSub::AnimDataCompilation, worker));		// 9 progress ups
							}
							else if (lowerFileName == "animationsetdatasinglefile.txt")
							{
								boost::asio::post(mt, boost::bind(&BehaviorSub::ASDCompilation, worker));
							}
							else if (temppath.find("characters") != 0)
							{
								boost::asio::post(mt, boost::bind(&BehaviorSub::BehaviorCompilation, worker));
							}
							else
							{
								worker->isCharacter = true;
								boost::asio::post(mt, boost::bind(&BehaviorSub::BehaviorCompilation, worker));
							}

							++repeatcount;
						}
					}
				}
			}
		}
	}
	catch (exception& ex)
	{
		mt.stop();

		for (auto& each : behaviorSubList)
		{
			delete each;
		}

		throw ex;
	}

	{
		lock_guard<mutex> lg(cv2_m);
		--behaviorRun;
	}

	cv2.notify_one();

	mt.join();

	for (auto& each : behaviorSubList)
	{
		delete each;
	}
}

void BehaviorStart::milestoneStart()
{
	m_RunningThread = 1;
	failedBehaviors.clear();
	PatchReset();
	start_time = boost::posix_time::microsec_clock::local_time();
	DebugLogging("Nemesis Behavior Version: v" + GetNemesisVersion());
	string curdir = QCoreApplication::applicationDirPath().toStdString();
	replace(curdir.begin(), curdir.end(), '/', '\\');
	DebugLogging("Current Directory: " + curdir);
	DebugLogging("Data Directory: " + nemesisInfo->GetDataPath());
	DebugLogging("Skyrim Special Edition: " + string(SSE ? "TRUE" : "FALSE"));
	int counter = 0;

	connectProcess(this);
	string directory = "temp_behaviors";
	string fpdirectory = directory + "\\_1stperson";
	vecstr filelist;
	int include = 0;
	int add = 4;

	if (!isFileExist(directory))
	{
		boost::filesystem::create_directory(boost::filesystem::path(directory));
		ErrorMessage(6006);
	}

	DebugLogging("Detecting processes...");
	read_directory(directory, filelist);

	if (isFileExist(directory + "\\animationdatasinglefile.txt")) --add;

	if (isFileExist(directory + "\\animationsetdatasinglefile.txt")) --add;

	if (isFileExist(fpdirectory) && boost::filesystem::is_directory(fpdirectory))
	{
		vecstr fpfilelist;
		read_directory(fpdirectory, fpfilelist);

		for (auto& file : fpfilelist)
		{
			if (!boost::filesystem::is_directory(fpdirectory + "\\" + file) && file.find(".txt") == file.length() - 4)
			{
				++include;
			}
		}
	}

	for (auto& file : filelist)
	{
		if (!boost::filesystem::is_directory(directory + "\\" + file) && file.find(".txt") == file.length() - 4)
		{
			++include;
		}
	}

	filenum = (include * 10) + add;
}

void BehaviorStart::unregisterProcess(bool skip)
{
	if (!skip)
	{
		if (!error)
		{
			bool hasScripts = false;
			RunScript("scripts\\launcher\\end\\", hasScripts);

			if (hasScripts) interMsg("");

			if (error)
			{
				string msg = TextBoxMessage(1010);
				interMsg(msg);
				DebugLogging(msg);
			}
			else
			{
				string msg;
				bool ms = false;
				boost::posix_time::time_duration diff = boost::posix_time::microsec_clock::local_time() - start_time;

				if (ms)
				{
					DebugLogging("Number of animations: " + to_string(animCount));
					int second = diff.total_milliseconds();

					if (second > 1000)
					{
						string milli = to_string(second % 1000);

						while (milli.length() < 3)
						{
							milli.insert(0, "0");
						}

						msg = TextBoxMessage(1009) + ": " + to_string(second / 1000) + "," + milli + " " + TextBoxMessage(1011);
					}
					else
					{
						msg = TextBoxMessage(1009) + ": " + to_string(second) + " " + TextBoxMessage(1011);
					}
				}
				else
				{
					DebugLogging("Number of animations: " + to_string(animCount));
					msg = TextBoxMessage(1009) + ": " + to_string(diff.total_seconds()) + " " + TextBoxMessage(1012);
				}

				interMsg(msg);
				DebugLogging(msg);
				emit disableCheck(false);
			}
		}
		else
		{
			string msg = TextBoxMessage(1010);
			interMsg(msg);
			DebugLogging(msg);
		}
	}

	DebugOutput();
	disconnectProcess();

	if (cmdline)
	{
		lock_guard<mutex>lock(processlock);
		processdone = true;
		cv.notify_one();
	}
	else
	{
		emit disable(false);
		emit hide(true);
	}

	emit end();
}

void BehaviorStart::EndAttempt()
{
	while (atomic_lock.test_and_set(boost::memory_order_acquire));

	--m_RunningThread;

	if (m_RunningThread == 0)
	{
		try
		{
			for (int i = 0; i < failedBehaviors.size(); i+=2)
			{
				hkxcmdProcess(failedBehaviors[i], failedBehaviors[i + 1], true);
				DebugLogging("Processing behavior: " + failedBehaviors[i] + " (Check point #, Behavior compile complete)");
				emit progressUp();
			}

			for (auto& msg : fileCheckMsg)
			{
				interMsg(msg);
			}

			failedBehaviors.clear();
			behaviorCheck();
		}
		catch (...) {}

		if (error)
		{
			ClearGlobal();
		}
		else
		{
			ClearGlobal(false);

			if (!cmdline) std::this_thread::sleep_for(std::chrono::milliseconds(1500));
		}

		try
		{
			unregisterProcess();
			p_terminate->exitSignal();
		}
		catch (...) {}
	}

	atomic_lock.clear(boost::memory_order_release);
}

void BehaviorStart::increaseAnimCount()
{
	if (!error)
	{
		lock_guard<mutex> lock(anim_lock);
		++animCount;
		emit totalAnim(animCount);
	}
}

void BehaviorStart::newMilestone()
{
	Lockless lock(upFlag);

	if (!error)
	{
		emit progressUp();
	}
}

void addOnInstall(string templine, string& elementLine, vector<vecstr>& groupAddOnElement, ImportContainer& addOn,
	unordered_map<string, unordered_map<string, vecstr>>& groupAddOn)
{
	for (auto it = addOn.begin(); it != addOn.end(); ++it)
	{
		for (auto iter = it->second.begin(); iter != it->second.end(); ++iter)
		{
			if (nemesis::iequals(templine, it->first + "[" + iter->first + "]"))
			{
				elementLine.replace(elementLine.find("$$"), 2, iter->second);
				return;
			}
		}
	}

	for (auto it = groupAddOn.begin(); it != groupAddOn.end(); ++it)
	{
		for (auto iter = it->second.begin(); iter != it->second.end(); ++iter)
		{
			if (nemesis::iequals(templine, it->first + "[" + iter->first + "]"))
			{
				elementLine.replace(elementLine.find("$$"), 2, "##");
				groupAddOnElement.push_back(iter->second);
				return;
			}

			groupAddOnElement.push_back(iter->second);
		}
	}
}
