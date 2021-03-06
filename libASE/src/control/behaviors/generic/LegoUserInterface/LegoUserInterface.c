


#define	  ASE_CTRL

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

//#include <ase/targets/non-public/lpf.h>
#include <ase/targets/AbstractModuleApi.h>
#include <ase/tools/Timer/TimerManager.h>
#include <ase/infrastructure/Scheduler/Scheduler.h>
#include <ase/infrastructure/EventManager/EventManager.h>
#include <ase/communication/Message.h>
#include <ase/control/strategies/kNN/kNN.h>
#include <ase/control/strategies/Playback/Playback.h>
#include <ase/control/arbitration/Subsumption/Subsumption.h>
#include <ase/control/behaviors/generic/LegoUserInterface/LuiManager.h>
#include <ase/control/behaviors/generic/LegoUserInterface/LuiBoard.h>
#include <ase/control/behaviors/generic/LegoUserInterface/LuiEventManager.h>
#include <ase/control/behaviors/generic/LegoUserInterface/LuiBehaviorManager.h>
#include <ase/control/behaviors/generic/LegoUserInterface/LuiTraining.h>
#include <ase/control/behaviors/generic/LegoUserInterface/LegoUserInterface.h>



/*#define LPF_READ_DEVICELIST	1
#define LPF_COUNT_DEVICELIST	2
#define LPF_STATE_LIST		3*/

#define STATE_RC	1
#define STATE_TRAIN	2
#define STATE_EXECUTE	3
#define STATE_RECORD	4
#define STATE_PLAYBACK	5
#define STATE_BEHAVIORS	6




static Subsumption_t SubsumptionProcess;
static Timer_t* boardTimer;
static Timer_t* soundTimer;
static BoardState_t board;


//void controller_act(char* topic, Event_t* event)  { }

static int priority;
void addtoSubsumption(char behaviorId) {
	if(behaviorId == 0) return;
	if(!LuiBehaviorManager_isCompound(behaviorId)) { 		//add simple behavior to subsumption
	  	//ase_printf("%i, ",behaviorId);
		Subsumption_setActive(behaviorId, 1, &SubsumptionProcess);
		Subsumption_setPriority(behaviorId, priority++, &SubsumptionProcess);
	}
	else {						//udfold compound behavior
		signed char selectedBehaviors[5];
		signed char knnInput[10];
		int nKnnInput  = LuiTraining_createKnnInput(LuiManager_getDeviceReadList(), LuiManager_getNumberOfInputDevices(), knnInput);
		kNN_t* knn = (kNN_t*) LuiBehaviorManager_getData(behaviorId);
		kNN_getOutput(knn, knnInput, nKnnInput, selectedBehaviors, 5);
		int i;
		ase_printf("Selected behaviors: ");
		for(i=0;i<5;i++) {
			ase_printf("%i, ",selectedBehaviors[i]);
			addtoSubsumption(selectedBehaviors[i]);
		}
		ase_printf("\n");
	}	
}

typedef struct {
	bool userError;
	bool systemError;
	bool training;
	bool brickFull;
} LuiState_t;
LuiState_t luiState;



void sound_timer_fired(int id) { //1 hz
	if(luiState.userError) {
		ase_printf("#play user-error.wav 50 1\n");
		luiState.userError = false;
	}
	if(luiState.training) {
	  	//ase_printf("#play learning.wav 50 1\n");
		luiState.training =  false;
	}
	if(luiState.brickFull) {
		ase_printf("#play fullBrick.wav 50 1\n");
		luiState.brickFull =  false;
	}
	if(luiState.systemError) { 
	  	luiState.systemError = false;
	}
}

void updateBrickStatus(int result) {
	if(result == 0) {
    	luiState.training = false;
		luiState.brickFull = false;
	}
	if(result == 1) {
		luiState.training = true;
		luiState.brickFull = false;
	}
	if(result == 2) {
		luiState.training = false;
		luiState.brickFull = true;
	}
}

void doSubsumptionBehaviors(signed char* behaviors, unsigned char nBehaviors) {
	Subsumption_deactivateAll(&SubsumptionProcess);
	priority = 0;
	//ase_printf("B: ");
	int i;
	for(i=0;i<nBehaviors;i++) {
		addtoSubsumption(behaviors[i]);
	}
	//ase_printf("\n");
	int nOutputs = LuiManager_getNumberOfOutputDevices();
	int nInputs = LuiManager_getNumberOfInputDevices();
	signed char output[MAX_KNN_OUTPUTS];
	Subsumption_act(LuiManager_getDeviceReadList(), nInputs, output, nOutputs, &SubsumptionProcess);
	LuiManager_applyControlOutput(output, nOutputs);
}

static void doNothing() {
	int nOutputs = LuiManager_getNumberOfOutputDevices();
	int nInputs = LuiManager_getNumberOfInputDevices();
	signed char output[nOutputs];
	Subsumption_deactivateAll(&SubsumptionProcess);
	Subsumption_act(LuiManager_getDeviceReadList(), nInputs, output, nOutputs, &SubsumptionProcess);
	LuiManager_applyControlOutput(output, nOutputs);
	ase_printf("Do nothing...\n");
}

void board_timer_fired(int id) { //10hz
  	LuiManager_updateDeviceReadList();
	if(!LuiManager_deviceReadSuccess()) {
		ase_printf("---READ ERROR---\n");
		luiState.systemError=true;
		return;
	}
  	int nOutputs = LuiManager_getNumberOfOutputDevices();
	if(board.start!=0) {
		if(board.cTrain!=0) {
		  	if(LuiBehaviorManager_isCompound(board.cTrain)) {
				if(!LuiEventManager_isWaitingForEvent()) {
					int result = LuiTraining_knn_train(LuiBehaviorManager_getData(board.cTrain), LuiManager_getSelectedBehaviorList(), 5);
					updateBrickStatus(result);
					if(result!=2) doSubsumptionBehaviors(LuiManager_getSelectedBehaviorList(), 5);
					else doNothing();
				}
				else {
					doNothing();
				}
			}
			else {
				luiState.userError = true;
			}
		}
		else if(board.train!=0) { //sample kNN training data
			if(LuiBehaviorManager_isTrain(board.train)) {
				if(!LuiEventManager_isWaitingForEvent()) {
					int result = LuiTraining_knn_train(LuiBehaviorManager_getData(board.train),LuiManager_getDeviceRCList(), nOutputs);
					updateBrickStatus(result);
					if(result!=2) LuiManager_applyControlOutput(LuiManager_getDeviceRCList(), nOutputs);
					else doNothing();
				}
				else {
					doNothing();
				}
			}
			else {
				luiState.userError = true;
			}
		}
		else if(board.record!=0) {
			if(LuiBehaviorManager_isRecord(board.record)) { //10hz upto 100 samples = max 10 sec recording
				Playback_record_if_novel(LuiBehaviorManager_getData(board.record),LuiManager_getDeviceRCList(), nOutputs);
				if(!Playback_isFull(LuiBehaviorManager_getData(board.record))) {
					LuiManager_applyControlOutput(LuiManager_getDeviceRCList(),nOutputs);
					updateBrickStatus(1);
				}
				else {
					doNothing();
					updateBrickStatus(2);
				}
			}
			else {
				luiState.userError = true;
			}
		}
		else if(board.clear!=0) {

		}
		else if(LuiBoard_hasBehavior(&board) || Subsumption_hasActiveBehaviors(&SubsumptionProcess)) {
		  	doSubsumptionBehaviors((signed char*)board.behaviors, 5);
		}
		else { //use gamepad if board is empty
			LuiManager_applyControlOutput(LuiManager_getDeviceRCList(),nOutputs);
		}
	}
	else {
		doNothing();
	}
}

static void handleMessage(char* topic, Event_t* event) {
	Msg_t* msg = (Msg_t*) event->val_prt;
    //ase_printf("BOT got message of type %i with length %i on channel %i\n",msg->message[0],msg->messageSize,msg->channel);
    int length = msg->messageSize;
	int type;
	if(length>0) type = msg->message[0];
	else return;
	if(type==LUI_SET_RC_STATE) {
		LuiManager_setDeviceRCList((signed char*)(&msg->message[1]), msg->messageSize-1, -100);
	}
	else if(type==LUI_UPDATE_DEVICELIST) {
		//int msgLength = LuiManager_updateDeviceCountList();
		//LPFApi_setMessage( (char*)LuiManager_getDeviceCountList(), msgLength);
	} 
	else if(type==LUI_UPDATE_READ_DEVICELIST) {
		//int msgLength = LuiManager_updateDeviceReadList();
		//LPFApi_setMessage((char*)LuiManager_getDeviceReadList(), msgLength);
	}
	else if(type==LUI_UPDATE_STATELIST) {
		/*char stateList[3];
		stateList[0] = LPF_STATE_LIST;
		stateList[1] = 0;//kNN_behavior.nSets;
		stateList[2] = 0;//playback_data.nSets;
		LPFApi_setMessage(stateList, 3);*/
	}
	else if(type==LUI_SELECTED_BEHAVIORS) {
		signed char behaviors[5];
		int i;
		for(i=0;i<5;i++) {
		  if(msg->message[i+1]==1) {
			behaviors[i] = LuiBoard_behaviorOf(i, &board);
		  }
		  else {
			behaviors[i] = 0;
		  }
		}
		LuiManager_setSelectedBehaviorList(behaviors);
	}
	else if(type==LUI_SETUP) {
		if(msg->messageSize>=11) {
			int i;
			for(i=0;i<5;i++) {
				//board.behaviors[i] = msg->message[i+1];
				if(LuiBoard_setBehavior(msg->message[i+1],i, &board)) {
					if(board.behaviors[i]!=0) {
						ase_printf("#play brickOn.wav 50 1\n");
					}
					else {
						ase_printf("#play brickOff.wav 50 1\n");
					}
				}
			}
			
			char start = msg->message[6];
			if(start!=board.start ) {
				//board.start = start;
				if(LuiBoard_setStart(start, &board)) {
					if(board.start!=0) {
						ase_printf("#play brickOn.wav 50 1\n");
					}
					else {
						ase_printf("#play brickOff.wav 50 1\n");
					}
				}
				if(start) {
					ase_printf("Board Started\n");
				}
				else {
					ase_printf("Board Stopped\n");
				}
			}
			
			char clear = msg->message[7];
			if(clear!=board.clear) {
			  	//board.clear = clear;
				if(LuiBoard_setClear(clear, &board)) {
					if(board.clear!=0) {
						ase_printf("#play brickOn.wav 50 1\n");
					}
					else {
						ase_printf("#play brickOff.wav 50 1\n");
					}
				}
				if(LuiBehaviorManager_isTrain(board.clear))  {
					kNN_clearMemory(LuiBehaviorManager_getData(board.clear));
					ase_printf("kNN memory cleared\n");
				}	
				else if(LuiBehaviorManager_isRecord(board.clear)) {
					Playback_clearRecordings(LuiBehaviorManager_getData(board.clear));
					ase_printf("Playback memory cleared\n");
				}
				else if(LuiBehaviorManager_isCompound(board.clear))  {
					kNN_clearMemory(LuiBehaviorManager_getData(board.clear));
					ase_printf("Compound memory cleared\n");
				}
				else if(LuiBehaviorManager_isSimple(board.clear))  {
					if(!LuiBehaviorManager_isEmpty(board.clear)) luiState.userError = true;
				}
			}
			
			char record = msg->message[8];
			if(record!=board.record) {
			  	//board.record = record;
				if(LuiBoard_setRecord(record, &board)) {
					if(board.record!=0) {
						ase_printf("#play brickOn.wav 50 1\n");
					}
					else {
						ase_printf("#play brickOff.wav 50 1\n");
					}
				}
				if(LuiBehaviorManager_isRecord(board.record)) {
					ase_printf("Playback started recording\n");
					Playback_startRecording(LuiBehaviorManager_getData(board.record));
				}
				else {
					if(!LuiBehaviorManager_isEmpty(board.record)) luiState.userError = true;
				}
			}
			
			char train = msg->message[9];
			if(train!=board.train) {
			  	//board.train = train;
				if(LuiBoard_setTrain(train, &board)) {
					if(board.train!=0) {
						ase_printf("#play brickOn.wav 50 1\n");
					}
					else {
						ase_printf("#play brickOff.wav 50 1\n");
					}
				}
				if(LuiBehaviorManager_isTrain(board.train)) {
					ase_printf("Simple Behavior Training Started (kNN)\n");
				  	LuiEventManager_waitForEvent();
				}
				else {
					if(!LuiBehaviorManager_isEmpty(board.record)) luiState.userError = true;
				}
			}
			char cTrain = msg->message[10];
			if(cTrain!=board.cTrain) {
			  	//board.cTrain = cTrain;
			  	if(LuiBoard_setCTrain(cTrain, &board)) {
					if(board.cTrain!=0) {
						ase_printf("#play brickOn.wav 50 1\n");
					}
					else {
						ase_printf("#play brickOff.wav 50 1\n");
					}
				}
				//if(LuiBehaviorManager_isTrain(board.cTrain)) {
			  	if(LuiBehaviorManager_isCompound(board.cTrain)) {
					ase_printf("Compound Behavior Training Started (kNN)\n");
				  	LuiEventManager_waitForEvent();
				}
				else {
					if(!LuiBehaviorManager_isEmpty(board.cTrain)) luiState.userError = true;
				}
				ase_printf("Compound Behavior Training Started %i\n",cTrain);
			}
		}
	}
}

Subsumption_t* LUI_getSubsumptionProcess() {
	return  &SubsumptionProcess;
}

void LUI_init() {
	LuiManager_init();
	LuiEventManager_init();
	LuiBoard_init(&board);
	Subsumption_init(&SubsumptionProcess);
	//boardTimer = TimerManager_createPeriodicTimer(100, 0, board_timer_fired);
	boardTimer = TimerManager_createPeriodicTimer(25, 0, board_timer_fired);
	soundTimer = TimerManager_createPeriodicTimer(1000, 1, sound_timer_fired);
	EventManager_subscribe(MSG_RECV_EVENT, handleMessage);
	ase_printf("Lego User Interface Initialized\n");
}
