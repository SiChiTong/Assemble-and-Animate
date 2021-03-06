#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <ase/targets/atron/AtronApi.h>
#include <ase/tools/IDContainer/IDContainer.h>
#include <ase/communication/RemoteControl/ModularCommander/MCManager.h>
#include <ase/control/strategies/Gradient/GradientManager.h>
#include <ase/CentralPatternGenerator.h>
#include <ase/targets/AbstractModuleApi.h>
#include <ase/CPGManager.h>
#include <ase/communication/CommDefs.h>
#include <ase/BroadCaster.h>
#include <ase/control/strategies/DistributedStateMachine/DistributedStateManager.h>
#include <ase/SnakeToQuadruped.h>
#include <ase/control/behaviors/atron/locomotion/SnakeGait/SnakeGait.h>
#include <ase/tools/Timer/TimerManager.h>
#include <ase/communication/StateSharing/StateSharing.h>
#include <ase/communication/Gossip/GossipManager.h>

#define N_MODULES	4

int internalState = 0;
int oldState=0;
gradient_struct* IDGradient;
timer_struct* timer;
void CommandHandler(uint8_t mcType, uint8_t* payload, uint8_t length, uint8_t channel);
void Timer_Fired(int id);

void controller_init() {
	printf("Running\n");
	srand (getRandomSeed());
	MCManager_init();
	IDContainer_init();
	broadcaster_init();
	SnakeGait_init();
	DSManager_init(-1);
	TimerManager_init();
	GossipManager_init();
	StateSharing_init();
	timer = TimerManager_createPeriodicTimer(250, 0, Timer_Fired);
	IDGradient = GradientManager_createGradient(1, 2);
	int i;
	for(i=1;i<=5;i++)	MCManager_installHandler(i, CommandHandler);
	atronApi_setLeds(255);
}

void Timer_Fired(int id) {
	//atronApi_setLeds(~atronApi_getLeds());
}

void CommandHandler(uint8_t mcType, uint8_t* payload, uint8_t length, uint8_t channel) {
	//printf("Got callback, payload length = %i \n", length);
	if(mcType==1) { //control if paused
		setPaused(payload[0]);
		printf("Pause = %i \n", isPaused());
	}
	else if(mcType==2) { //set gradient seed
		IDGradient->seed = N_MODULES;
		printf("Gradient seed = %i \n", IDGradient->seed);
	}
	else if(mcType==3) { //set command
		if(getState()!=payload[0]) {
			setState(payload[0]);
		}
		printf("Current command set = %i \n", getState());
	}
	else if(mcType==4) { //set command
		atronApi_setLeds(payload[0]);
		printf("Leds set = %i \n", payload[0]);
	}
	else if(mcType==5) { //change snake behavior
		if(length<2) return;
		if(payload[0]==1) {
			float x = 1.412f*payload[1]; //scale to 0-360
			SnakeGait_setAmplitude(x);
		}
		if(payload[0]==2) {
			float x = 1.412f*payload[1]; //scale to 0-360
			SnakeGait_setOffset(x);
		}
		if(payload[0]==3) {
			float x = 0.0246399424*payload[1]; //scale to 0 - 2*PI degree
			SnakeGait_setPhaseShift(x);
		}
		if(payload[0]==4) {
			float x = 0.01f*payload[1]; //scale to 0.01 hz - 2.55 hz
			SnakeGait_setFrequency(x);
		}
		printf("Changed snakes behavior = %i \n", payload[0]);
	}
	if(channel==8) { // message from serial interface or zigbee is always the "newest"
		setTimeStamp(getTimeStamp()+1);
	}
}

int setIDFromGradient() {
	if(IDContainer_getSoftID()==-1) {
		int val = gradient_getValue(IDGradient);
		if(val!=0) {
			IDContainer_setSoftID(val);
			printf("I got ID = %i\n", IDContainer_getSoftID());
			atronApi_setLeds(val);
			return 1;
		}
		return 0;
	}
	return 1;
}


void controller_act()  {
	if(oldState!=getState()) {
		internalState = 0;
	}
	oldState = getState();
	if(!isPaused()) {
		if(getState() == 1) { //assign IDs from gradient (assumes snake topology)
			int success = setIDFromGradient();
			if(GradientManager_act()>0) { }
		}
		else if(getState() == 2) { //move as snake
			SnakeGait_act();
		}
		else if(getState() == 3) { //self-reconfigure to quadruped
			if(internalState==0) {
				SnakeToQuadruped_init(IDContainer_getSoftID());
				internalState++;
			}
			else if(internalState==1) {
				if(!SnakeToQuadruped_isDone()) {
					SnakeToQuadruped_act();
				}
				else {
					internalState = 0;
				}
			}
			if(DSManager_act()>0) { }
		}
		else if(getState() == 4) { //move as quadruped
		}
		else if(getState() == 5) { //self-reconfigure to walker from quadruped
		}
		else if(getState() == 6) { //move as quadruped
		}
		else if(getState() == 7) { //self-reconfigure to snake from quadruped
		}
		else if(getState() == 8) { //home module
			if(internalState==0) {
				atronApi_rotateToDegree(0);
				internalState=1;
			}
		}
		else if(getState() == 9) { //disconnect all
			if(internalState==0) {
				atronApi_disconnectAll();
				internalState=1;
			}
		}
	}
	atronApi_yield();
	if(broadcaster_act()>0) { }
	if(MCManager_act()>0) { }
	if(TimerManager_act()>0) { }
	if(GossipManager_act()>0) { }
}

void handleMessage(char* message, char messageSize, char channel) {
	/*if(channel!=8) {
		if((rand()%100)>50) {
			return;
		}
	}*/
	atronApi_setLeds(~atronApi_getLeds()); //makes ussr crach

	if(message[0]==MODULAR_COMMANDER_MESSAGE) {
		MCManager_handleMessage(message, messageSize, channel);
	}
	else if(message[0]==CPG_MESSAGE) {
		CPGManager_handleMessage(message, messageSize, channel);
	}
	else if(message[0]==GRADIENT_MESSAGE) {
		GradientManager_handleMessage(message, messageSize, channel);
	}
	else if(message[0]==DISTRIBUTED_STATE_MESSAGE) {
		DSManager_handleMessage(message, messageSize, channel);
	}
	else if(message[0]==GOSSIP_MESSAGE) {
		GossipManager_handleMessage(message, messageSize, channel);
	}
	else {
		printf("Warning: Unrecognized message type received %i \n",message[0]);
	}
}
