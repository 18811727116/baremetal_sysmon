#include "xsysmonpsu.h"
#include "xparameters.h"
#include "xstatus.h"
#include "xil_exception.h"
#include "xscugic.h"
#include "xil_printf.h"
#define printf xil_printf 	// Small foot-print printf function
#define SYSMON_DEVICE_ID	XPAR_XSYSMONPSU_0_DEVICE_ID
#define XSCUGIC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID  //SCUGIC Interrupt Controller
#define INTR_ID			XPAR_XSYSMONPSU_INTR //INTR_ID = XPAR_XSYSMONPSU_INTR = XPS_AMS_INT_ID = (56U + 32U)

int PS_SysMonPsuIntrExample(XScuGic* XScuGicInstPtr, XSysMonPsu* SysMonInstPtr, u16 SysMonDeviceId, u16 SysMonIntrId);
int PL_SysMonPsuIntrExample(XScuGic* XScuGicInstPtr, XSysMonPsu* SysMonInstPtr, u16 SysMonDeviceId, u16 SysMonIntrId);
static void SysMonPsuInterruptHandler(void *CallBackRef);
static int SysMonPsuSetupInterruptSystem(XScuGic* XScuGicInstPtr, XSysMonPsu *SysMonPtr, u16 IntrId );
static int SysMonPsuFractionToInt(float FloatNum);


#ifndef TESTAPP_GEN
static XSysMonPsu SysMonInst; 		// System Monitor driver instance
static XScuGic XScuGicInst; 			// Instance of the XXScuGic driver
#endif

//PS SYSMON Sensor Channel
volatile static int TempLPIntrActive = FALSE;
volatile static int TempFPIntrActive = FALSE;
volatile static int VCC_PSINTLPIntr = FALSE;
volatile static int VCCINTFPIntr = FALSE;
volatile static int VCC_PSAUXIntr = FALSE;
volatile static int VCC_PSINTFP_DDR_504Intr = FALSE;
volatile static int VCC_PSIO3_503Intr = FALSE;
volatile static int VCC_PSIO0_500Intr = FALSE;
volatile static int VCC_PSIO1_501Intr = FALSE;
volatile static int VCC_PSIO2_502Intr = FALSE;
volatile static int PS_MGTRAVCCIntr = FALSE;
volatile static int PS_MGTRAVTTIntr = FALSE;
volatile static int VCCAMSIntr = FALSE;
//PL_Temp,PL_VCCINT,PL_VCCAUX,PL_VP_VN,PL_VREFP,PL_VREFN,PL_VCCBRAM,PL_VCC_PSINTLP,PL_VCC_PSINTFP,PL_VCC_PSAUX,PL_VAUXP1,PL_VUser0,PL_VCCADC
//PL SYSMON Sensor Channel
volatile static int PL_TempActive = FALSE;
volatile static int PL_VCCINTIntr = FALSE;
volatile static int PL_VCCAUXIntr = FALSE;
volatile static int PL_VP_VNIntr = FALSE;
volatile static int PL_VREFPIntr = FALSE;
volatile static int PL_VREFNIntr = FALSE;
volatile static int PL_VCCBRAMIntr = FALSE;
volatile static int PL_VCC_PSINTLPIntr = FALSE;
volatile static int PL_VCC_PSINTFPIntr = FALSE;
volatile static int PL_VCC_PSAUXIntr = FALSE;
volatile static int PL_VAUXP1Intr = FALSE;
volatile static int PL_VUser0Intr = FALSE;
volatile static int PL_VUser1Intr = FALSE;
volatile static int PL_VUser2Intr = FALSE;
volatile static int PL_VUser3Intr = FALSE;
volatile static int PL_VCCADCIntr = FALSE;
//AMSVCC_PSPLL,AMSVCC_PSBATT,AMSVCCINT,AMSVCCBRAM,AMSVCCAUX,AMSVCC_PSDDR_PLL,AMSVCC_DDRPHY_REF,AMSVCC_PSINTFO_DDR
//Measurement Registers in AMS(single-channel mode, sequencer off, unipolar sampling circuit, not have alarms or minimum/maximum registers.)
volatile static int AMSVCC_PSPLLIntr = FALSE;
volatile static int AMSVCC_PSBATTIntr = FALSE;
volatile static int AMSVCCINTIntr = FALSE;
volatile static int AMSVCCBRAMIntr = FALSE;
volatile static int AMSVCCAUXIntr = FALSE;
volatile static int AMSVCC_PSDDR_PLLIntr = FALSE;
volatile static int AMSVCC_DDRPHY_REFIntr = FALSE;
volatile static int AMSVCC_PSINTFO_DDRIntr = FALSE;


#ifndef TESTAPP_GEN


int main(void)
{
	int Status;

	//while(1)
	//{
		Status = PS_SysMonPsuIntrExample(&XScuGicInst, &SysMonInst, SYSMON_DEVICE_ID, INTR_ID);//INTR_ID = XPAR_XSYSMONPSU_INTR = XPS_AMS_INT_ID = (56U + 32U)
		if (Status != XST_SUCCESS)
		{
			xil_printf("PS_SysMonPsu Interrupt Example Test Failed\r\n");
			return XST_FAILURE;
		}
		Status = PL_SysMonPsuIntrExample(&XScuGicInst, &SysMonInst, SYSMON_DEVICE_ID, INTR_ID);
		if (Status != XST_SUCCESS)
		{
			xil_printf("PL_SysMonPsu Interrupt Example Test Failed\r\n");
			return XST_FAILURE;
		}
		xil_printf("Successfully ran SysMonPsu Interrupt Example Test\r\n");
	//	sleep(5);
	//}
	return XST_SUCCESS;
}
#endif

/****************************************************************************/
/**
*
* This function runs a test on the System Monitor device using the
* driver APIs.
*
* The function does the following tasks:
*	- Initiate the System Monitor device driver instance
*	- Run self-test on the device
*	- Reset the device
*	- Set up alarms for on-chip temperature, VCCINT and VCCAUX
*	- Set up sequence registers to continuously monitor on-chip
*	temperature, VCCINT and  VCCAUX
*	- Setup interrupt system
*	- Enable interrupts
*	- Set up configuration registers to start the sequence
*	- Wait until temperature alarm interrupt or VCCINT alarm interrupt
*	or VCCAUX alarm interrupt occurs
*
* @param	XScuGicInstPtr is a pointer to the Interrupt Controller
*		driver Instance.
* @param	SysMonInstPtr is a pointer to the XSysMonPsu driver Instance.
* @param	SysMonDeviceId is the XPAR_<SYSMON_instance>_DEVICE_ID
*		value from xparameters.h.
* @param	SysMonIntrId is
*		XPAR_<SYSMON_instance>_INTR value from xparameters_ps.h
*
* @return
*		- XST_SUCCESS if the example has completed successfully.
*		- XST_FAILURE if the example has failed.
*
* @note		This function may never return if no interrupt occurs.
*
****************************************************************************/
int PS_SysMonPsuIntrExample(XScuGic* XScuGicInstPtr, XSysMonPsu* SysMonInstPtr, u16 SysMonDeviceId, u16 SysMonIntrId)//SysMonIntrId = INTR_ID, INTR_ID = XPAR_XSYSMONPSU_INTR = XPS_AMS_INT_ID = (56U + 32U)
{
	int Status;
	XSysMonPsu_Config *ConfigPtr;
	u64 IntrStatus;
	u32 Data;
	u32 TempLPRawData, TempFPRawData, VCC_PSINTLPRawData, VCCINTFPRawData, VCC_PSAUXRawData, VCC_PSINTFP_DDR_504RawData, VCC_PSIO3_503RawData, \
	VCC_PSIO0_500RawData, VCC_PSIO1_501RawData, VCC_PSIO2_502RawData, PS_MGTRAVCCRawData, PS_MGTRAVTTRawData, VCCAMSRawData;
	float TempLPData, TempFPData, VCC_PSINTLPData, VCCINTFPData, VCC_PSAUXData, VCC_PSINTFP_DDR_504Data, VCC_PSIO3_503Data, VCC_PSIO0_500Data, \
	VCC_PSIO1_501Data, VCC_PSIO2_502Data, PS_MGTRAVCCData, PS_MGTRAVTTData, VCCAMSData, MaxData, MinData;

	printf("Entering the PS_SysMonPsu Interrupt Example. \r\n");

	ConfigPtr = XSysMonPsu_LookupConfig(SysMonDeviceId);//Initialize the SysMon driver
	if (ConfigPtr == NULL)
	{
		return XST_FAILURE;
	}

	XSysMonPsu_CfgInitialize(SysMonInstPtr, ConfigPtr, ConfigPtr->BaseAddress);

	Status = XSysMonPsu_SelfTest(SysMonInstPtr);//Self Test the System Monitor device
	if (Status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}

	XSysMonPsu_SetSequencerMode(SysMonInstPtr, XSM_SEQ_MODE_SAFE, XSYSMON_PS);//Disable the Channel Sequencer before configuring the Sequence registers
	XSysMonPsu_SetAvg(SysMonInstPtr, XSM_AVG_16_SAMPLES, XSYSMON_PS);//Setup the Averaging to be done for the channels in the Configuration 0 register as 16 samples

	/*
	 * Setup the Sequence register for 1st Auxiliary channel
	 * Setting is:
	 *	- Add acquisition time by 6 ADCCLK cycles.
	 *	- Bipolar Mode
	 *
	 * Setup the Sequence register for 16th Auxiliary channel
	 * Setting is:
	 *	- Add acquisition time by 6 ADCCLK cycles.
	 *	- Unipolar Mode
	 */


	Status = XSysMonPsu_SetSeqInputMode(SysMonInstPtr, XSYSMONPSU_SEQ_CH1_VAUX00_MASK << 16, XSYSMON_PS);
	if (Status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}

	Status = XSysMonPsu_SetSeqAcqTime(SysMonInstPtr, (XSYSMONPSU_SEQ_CH1_VAUX0F_MASK | XSYSMONPSU_SEQ_CH1_VAUX00_MASK) << 16, XSYSMON_PS);
	if (Status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}


	//Enable the averaging on the following channels in the Sequencer registers:On-chip Temperature, Remote temperature,VCC_PSINTLP,VCC_PSINTFP,VCC_PSAUX,
	//VCC_PSDDR_504,VCCAMS,VCC_PSIO3_503,VCC_PSIO0_500,VCC_PSIO1_501,VCC_PSIO2_502,PS_MGTRAVCC,PS_MGTRAVTT
	Status =  XSysMonPsu_SetSeqAvgEnables(SysMonInstPtr, XSYSMONPSU_SEQ_CH0_TEMP_MASK |
	XSYSMONPSU_SEQ_CH0_SUP1_MASK | XSYSMONPSU_SEQ_CH0_SUP2_MASK |
	XSYSMONPSU_SEQ_CH0_SUP3_MASK | XSYSMONPSU_SEQ_CH0_SUP4_MASK |
	XSYSMONPSU_SEQ_CH0_SUP5_MASK | XSYSMONPSU_SEQ_CH0_SUP6_MASK |
	(XSYSMONPSU_SEQ_CH2_TEMP_RMT_MASK | XSYSMONPSU_SEQ_CH2_SUP7_MASK | XSYSMONPSU_SEQ_CH2_SUP8_MASK |
	XSYSMONPSU_SEQ_CH2_SUP9_MASK | XSYSMONPSU_SEQ_CH2_SUP10_MASK |(u64)(XSYSMONPSU_SEQ_CH2_VCCAMS_MASK)) << 32, XSYSMON_PS);

	if (Status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}


	//Enable the averaging on the following channels in the Sequencer registers:On-chip Temperature, Remote temperature,VCC_PSINTLP,VCC_PSINTFP,VCC_PSAUX,
	//VCC_PSDDR_504,VCCAMS,VCC_PSIO3_503,VCC_PSIO0_500,VCC_PSIO1_501,VCC_PSIO2_502,PS_MGTRAVCC,PS_MGTRAVTT
	Status =  XSysMonPsu_SetSeqChEnables(SysMonInstPtr, XSYSMONPSU_SEQ_CH0_TEMP_MASK |
	XSYSMONPSU_SEQ_CH0_SUP1_MASK | XSYSMONPSU_SEQ_CH0_SUP2_MASK |
	XSYSMONPSU_SEQ_CH0_SUP3_MASK | XSYSMONPSU_SEQ_CH0_SUP4_MASK |
	XSYSMONPSU_SEQ_CH0_SUP5_MASK | XSYSMONPSU_SEQ_CH0_SUP6_MASK |
	(XSYSMONPSU_SEQ_CH2_TEMP_RMT_MASK | XSYSMONPSU_SEQ_CH2_SUP7_MASK | XSYSMONPSU_SEQ_CH2_SUP8_MASK |
	XSYSMONPSU_SEQ_CH2_SUP9_MASK | XSYSMONPSU_SEQ_CH2_SUP10_MASK |(u64)(XSYSMONPSU_SEQ_CH2_VCCAMS_MASK)) << 32, XSYSMON_PS);

	if (Status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}

	IntrStatus = XSysMonPsu_IntrGetStatus(SysMonInstPtr);//Clear any bits set in the Interrupt Status Register
	XSysMonPsu_IntrClear(SysMonInstPtr, IntrStatus);

	XSysMonPsu_SetSequencerMode(SysMonInstPtr, XSM_SEQ_MODE_CONTINPASS, XSYSMON_PS);//Enable the Channel Sequencer in continuous sequencer cycling mode.

	// Wait till the End of Sequence occurs
	while ((XSysMonPsu_IntrGetStatus(SysMonInstPtr) & ((u64)XSYSMONPSU_ISR_1_EOS_MASK << 32)) != ((u64)XSYSMONPSU_ISR_1_EOS_MASK << 32));


	//Read the ADC converted Data from the data registers for on-chip temperature, on-chip VCCINT voltage and on-chip VCCAUX voltage.
	TempLPRawData = XSysMonPsu_GetAdcData(SysMonInstPtr, XSM_CH_TEMP, XSYSMON_PS);
	TempLPData = XSysMonPsu_RawToTemperature_OnChip(TempLPRawData);//Convert the Raw Data to Degrees Centigrade and Voltage
	printf("\r\nThe Current LP Temperature is %0d.%03d Centigrade.\r\n", (int)(TempLPData), SysMonPsuFractionToInt(TempLPData));
	TempLPRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MAX_TEMP, XSYSMON_PS);
	MaxData	= XSysMonPsu_RawToTemperature_OnChip(TempLPRawData);
	printf("The Maximum LP Temperature is %0d.%03d Centigrade. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	TempLPRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MIN_TEMP, XSYSMON_PS);
	MinData	= XSysMonPsu_RawToTemperature_OnChip(TempLPRawData);
	printf("The Minimum LP Temperature is %0d.%03d Centigrade. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	TempFPRawData = XSysMonPsu_GetAdcData(SysMonInstPtr, XSM_CH_TEMP_REMTE, XSYSMON_PS);
	TempFPData = XSysMonPsu_RawToTemperature_OnChip(TempFPRawData);
	printf("\r\nThe Current FP Temperature is %0d.%03d Centigrade.\r\n", (int)(TempFPData), SysMonPsuFractionToInt(TempFPData));
	TempFPRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MAX_TEMP_REMOTE, XSYSMON_PS);
	MaxData	= XSysMonPsu_RawToTemperature_OnChip(TempFPRawData);
	printf("The Maximum FP Temperature is %0d.%03d Centigrade. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	TempFPRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MAX_TEMP_REMOTE, XSYSMON_PS);
	MinData	= XSysMonPsu_RawToTemperature_OnChip(TempFPRawData);
	printf("The Minimum FP Temperature is %0d.%03d Centigrade. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	VCC_PSINTLPRawData = XSysMonPsu_GetAdcData(SysMonInstPtr, XSM_CH_SUPPLY1, XSYSMON_PS);
	VCC_PSINTLPData	= XSysMonPsu_RawToVoltage(VCC_PSINTLPRawData);
	printf("\r\nThe Current VCC_PSINTLP is %0d.%03d Volts. \r\n", (int)(VCC_PSINTLPData), SysMonPsuFractionToInt(VCC_PSINTLPData));
	VCC_PSINTLPRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MAX_SUPPLY1, XSYSMON_PS);
	MaxData	= XSysMonPsu_RawToVoltage(VCC_PSINTLPRawData);
	printf("The Maximum VCC_PSINTLP is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	VCC_PSINTLPRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MIN_SUPPLY1, XSYSMON_PS);
	MinData	= XSysMonPsu_RawToVoltage(VCC_PSINTLPRawData);
	printf("The Minimum VCC_PSINTLP is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	VCCINTFPRawData = XSysMonPsu_GetAdcData(SysMonInstPtr, XSM_CH_SUPPLY2, XSYSMON_PS);
	VCCINTFPData = XSysMonPsu_RawToVoltage(VCCINTFPRawData);
	printf("\r\nThe Current VCCINTFP is %0d.%03d Volts. \r\n", (int)(VCCINTFPData), SysMonPsuFractionToInt(VCCINTFPData));
	VCCINTFPRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MAX_SUPPLY2, XSYSMON_PS);
	MaxData	= XSysMonPsu_RawToVoltage(VCCINTFPRawData);
	printf("The Maximum VCCINTFP is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	VCCINTFPRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MIN_SUPPLY2, XSYSMON_PS);
	MinData	= XSysMonPsu_RawToVoltage(VCCINTFPRawData);
	printf("The Minimum VCCINTFP is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	VCC_PSAUXRawData = XSysMonPsu_GetAdcData(SysMonInstPtr, XSM_CH_SUPPLY3, XSYSMON_PS);
	VCC_PSAUXData = XSysMonPsu_RawToVoltage(VCC_PSAUXRawData);
	printf("\r\nThe Current VCC_PSAUX is %0d.%03d Volts. \r\n", (int)(VCC_PSAUXData), SysMonPsuFractionToInt(VCC_PSAUXData));
	VCC_PSAUXRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MAX_SUPPLY3, XSYSMON_PS);
	MaxData	= XSysMonPsu_RawToVoltage(VCC_PSAUXRawData);
	printf("The Maximum VCC_PSAUX is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	VCC_PSAUXRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MIN_SUPPLY3, XSYSMON_PS);
	MinData	= XSysMonPsu_RawToVoltage(VCC_PSAUXRawData);
	printf("The Minimum VCC_PSAUX is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	VCC_PSINTFP_DDR_504RawData = XSysMonPsu_GetAdcData(SysMonInstPtr, XSM_CH_SUPPLY4, XSYSMON_PS);
	VCC_PSINTFP_DDR_504Data = XSysMonPsu_RawToVoltage(VCC_PSINTFP_DDR_504RawData);
	printf("\r\nThe Current VCC_PSINTFP_DDR_504 is %0d.%03d Volts. \r\n", (int)(VCC_PSINTFP_DDR_504Data), SysMonPsuFractionToInt(VCC_PSINTFP_DDR_504Data));
	VCC_PSINTFP_DDR_504RawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MAX_SUPPLY4, XSYSMON_PS);
	MaxData	= XSysMonPsu_RawToVoltage(VCC_PSINTFP_DDR_504RawData);
	printf("The Maximum VCC_PSINTFP_DDR_504 is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	VCC_PSINTFP_DDR_504RawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MIN_SUPPLY4, XSYSMON_PS);
	MinData	= XSysMonPsu_RawToVoltage(VCC_PSINTFP_DDR_504RawData);
	printf("The Minimum VCC_PSINTFP_DDR_504 is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	VCC_PSIO3_503RawData = XSysMonPsu_GetAdcData(SysMonInstPtr, XSM_CH_SUPPLY5, XSYSMON_PS);
	VCC_PSIO3_503Data = XSysMonPsu_VccopsioRawToVoltage(VCC_PSIO3_503RawData);
	printf("\r\nThe Current VCC_PSIO3_503 is %0d.%03d Volts. \r\n", (int)(VCC_PSIO3_503Data), SysMonPsuFractionToInt(VCC_PSIO3_503Data));
	VCC_PSIO3_503RawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MAX_SUPPLY5, XSYSMON_PS);
	MaxData	= XSysMonPsu_VccopsioRawToVoltage(VCC_PSIO3_503RawData);
	printf("The Maximum VCC_PSIO3_503 is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	VCC_PSIO3_503RawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MIN_SUPPLY5, XSYSMON_PS);
	MinData	= XSysMonPsu_VccopsioRawToVoltage(VCC_PSIO3_503RawData);
	printf("The Minimum VCC_PSIO3_503 is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	VCC_PSIO0_500RawData = XSysMonPsu_GetAdcData(SysMonInstPtr, XSM_CH_SUPPLY6, XSYSMON_PS);
	VCC_PSIO0_500Data = XSysMonPsu_VccopsioRawToVoltage(VCC_PSIO0_500RawData);
	printf("\r\nThe Current VCC_PSIO0_500 is %0d.%03d Volts. \r\n", (int)(VCC_PSIO0_500Data), SysMonPsuFractionToInt(VCC_PSIO0_500Data));
	VCC_PSIO0_500RawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MAX_SUPPLY6, XSYSMON_PS);
	MaxData	= XSysMonPsu_VccopsioRawToVoltage(VCC_PSIO0_500RawData);
	printf("The Maximum VCC_PSIO0_500 is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	VCC_PSIO0_500RawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MIN_SUPPLY6, XSYSMON_PS);
	MinData	= XSysMonPsu_VccopsioRawToVoltage(VCC_PSIO0_500RawData);
	printf("The Minimum VCC_PSIO0_500 is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	VCC_PSIO1_501RawData = XSysMonPsu_GetAdcData(SysMonInstPtr, XSM_CH_SUPPLY7, XSYSMON_PS);
	VCC_PSIO1_501Data = XSysMonPsu_VccopsioRawToVoltage(VCC_PSIO1_501RawData);
	printf("\r\nThe Current VCC_PSIO1_501 is %0d.%03d Volts. \r\n", (int)(VCC_PSIO1_501Data), SysMonPsuFractionToInt(VCC_PSIO1_501Data));
	VCC_PSIO1_501RawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MAX_SUPPLY7, XSYSMON_PS);
	MaxData	= XSysMonPsu_VccopsioRawToVoltage(VCC_PSIO1_501RawData);
	printf("The Maximum VCC_PSIO1_501 is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	VCC_PSIO1_501RawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MIN_SUPPLY7, XSYSMON_PS);
	MinData	= XSysMonPsu_VccopsioRawToVoltage(VCC_PSIO1_501RawData);
	printf("The Minimum VCC_PSIO1_501 is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	VCC_PSIO2_502RawData = XSysMonPsu_GetAdcData(SysMonInstPtr, XSM_CH_SUPPLY8, XSYSMON_PS);
	VCC_PSIO2_502Data = XSysMonPsu_VccopsioRawToVoltage(VCC_PSIO2_502RawData);
	printf("\r\nThe Current VCC_PSIO2_502 is %0d.%03d Volts. \r\n", (int)(VCC_PSIO2_502Data), SysMonPsuFractionToInt(VCC_PSIO2_502Data));
	VCC_PSIO2_502RawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MAX_SUPPLY8, XSYSMON_PS);
	MaxData	= XSysMonPsu_VccopsioRawToVoltage(VCC_PSIO2_502RawData);
	printf("The Maximum VCC_PSIO2_502 is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	VCC_PSIO2_502RawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MIN_SUPPLY8, XSYSMON_PS);
	MinData	= XSysMonPsu_VccopsioRawToVoltage(VCC_PSIO2_502RawData);
	printf("The Minimum VCC_PSIO2_502 is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	PS_MGTRAVCCRawData = XSysMonPsu_GetAdcData(SysMonInstPtr, XSM_CH_SUPPLY9, XSYSMON_PS);
	PS_MGTRAVCCData	= XSysMonPsu_RawToVoltage(PS_MGTRAVCCRawData);
	printf("\r\nThe Current PS_MGTRAVCC is %0d.%03d Volts. \r\n", (int)(PS_MGTRAVCCData), SysMonPsuFractionToInt(PS_MGTRAVCCData));
	PS_MGTRAVCCRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MAX_SUPPLY9, XSYSMON_PS);
	MaxData	= XSysMonPsu_RawToVoltage(PS_MGTRAVCCRawData);
	printf("The Maximum PS_MGTRAVCC is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	PS_MGTRAVCCRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MIN_SUPPLY9, XSYSMON_PS);
	MinData	= XSysMonPsu_RawToVoltage(PS_MGTRAVCCRawData);
	printf("The Minimum PS_MGTRAVCC is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	PS_MGTRAVTTRawData = XSysMonPsu_GetAdcData(SysMonInstPtr, XSM_CH_SUPPLY10, XSYSMON_PS);
	PS_MGTRAVTTData	= XSysMonPsu_RawToVoltage(PS_MGTRAVTTRawData);
	printf("\r\nThe Current PS_MGTRAVTT is %0d.%03d Volts. \r\n", (int)(PS_MGTRAVTTData), SysMonPsuFractionToInt(PS_MGTRAVTTData));
	PS_MGTRAVTTRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MAX_SUPPLY10, XSYSMON_PS);
	MaxData	= XSysMonPsu_RawToVoltage(PS_MGTRAVTTRawData);
	printf("The Maximum PS_MGTRAVTT is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	PS_MGTRAVTTRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MIN_SUPPLY10, XSYSMON_PS);
	MinData	= XSysMonPsu_RawToVoltage(PS_MGTRAVTTRawData);
	printf("The Minimum PS_MGTRAVTT is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	VCCAMSRawData = XSysMonPsu_GetAdcData(SysMonInstPtr, XSM_CH_VCCAMS, XSYSMON_PS);
	VCCAMSData	= XSysMonPsu_RawToVoltage(VCCAMSRawData);
	printf("\r\nThe Current VCCAMS is %0d.%03d Volts. \r\n", (int)(VCCAMSData), SysMonPsuFractionToInt(VCCAMSData));
	VCCAMSRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MAX_VCCAMS, XSYSMON_PS);
	MaxData	= XSysMonPsu_RawToVoltage(VCCAMSRawData);
	printf("The Maximum VCCAMS is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	VCCAMSRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MIN_VCCAMS, XSYSMON_PS);
	MinData	= XSysMonPsu_RawToVoltage(VCCAMSRawData);
	printf("The Minimum VCCAMS is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));




	XSysMonPsu_SetAlarmEnables(SysMonInstPtr, 0, XSYSMON_PS);//Disable all the alarms in the Configuration Register 1.

	Status = SysMonPsuSetupInterruptSystem(XScuGicInstPtr, SysMonInstPtr, SysMonIntrId);//Set up the AMS interrupt system,INTR_ID = XPAR_XSYSMONPSU_INTR = XPS_AMS_INT_ID = (56U + 32U)
	if (Status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}


	IntrStatus = XSysMonPsu_IntrGetStatus(SysMonInstPtr);//Clear any bits set in the Interrupt Status Register.
	XSysMonPsu_IntrClear(SysMonInstPtr, IntrStatus);





	 // Set up Alarm threshold registers for On-chip Temperature High/Low limit, VCCINT High/Low limit,VCCAUX High/Low limit, so that the Alarms occur.
	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_TEMP_UPPER, XSysMonPsu_TemperatureToRaw_OnChip(TempLPData + 10), XSYSMON_PS);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_TEMP_UPPER, XSYSMON_PS);//Read the TempLP Temperature Alarm Threshold registers
	MaxData	= XSysMonPsu_RawToTemperature_OnChip(Data);
	printf("\r\nLP Temperature Alarm(0) HIGH Threshold is %0d.%03d Centigrade. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_TEMP_LOWER, XSysMonPsu_TemperatureToRaw_OnChip(TempLPData - 20), XSYSMON_PS);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_TEMP_LOWER, XSYSMON_PS);
	MinData = XSysMonPsu_RawToTemperature_OnChip(Data);
	printf("LP Temperature Alarm(0) LOW Threshold is %0d.%03d Centigrade. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP1_UPPER, XSysMonPsu_VoltageToRaw(VCC_PSINTLPData + 0.2), XSYSMON_PS);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP1_UPPER, XSYSMON_PS);//Read the VCCINT Alarm Threshold registers.
	MaxData	= XSysMonPsu_RawToVoltage(Data);
	printf("\r\nVCC_PSINTLP Alarm(1) HIGH Threshold is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
    XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP1_LOWER, XSysMonPsu_VoltageToRaw(VCC_PSINTLPData - 0.4), XSYSMON_PS);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP1_LOWER, XSYSMON_PS);
	MinData	= XSysMonPsu_RawToVoltage(Data);
	printf("VCC_PSINTLP Alarm(1) LOW Threshold is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP2_UPPER, XSysMonPsu_VoltageToRaw(VCCINTFPData + 0.2), XSYSMON_PS);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP2_UPPER, XSYSMON_PS);//Read the VCCINTFP Alarm Threshold registers.
	MaxData	= XSysMonPsu_RawToVoltage(Data);
	printf("\r\nVCCINTFP Alarm(2) HIGH Threshold is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
    XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP2_LOWER, XSysMonPsu_VoltageToRaw(VCCINTFPData - 0.4), XSYSMON_PS);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP2_LOWER, XSYSMON_PS);
	MinData	= XSysMonPsu_RawToVoltage(Data);
	printf("VCCINTFP Alarm(2) LOW Threshold is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP3_UPPER, XSysMonPsu_VoltageToRaw(VCC_PSAUXData + 0.2), XSYSMON_PS);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP3_UPPER, XSYSMON_PS);//Read the VCC_PSAUX Alarm Threshold registers.
	MaxData	= XSysMonPsu_RawToVoltage(Data);
	printf("\r\nVCC_PSAUX Alarm(3) HIGH Threshold is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP3_LOWER, XSysMonPsu_VoltageToRaw(VCC_PSAUXData - 0.4), XSYSMON_PS);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP3_LOWER, XSYSMON_PS);
	MinData	= XSysMonPsu_RawToVoltage(Data);
	printf("VCC_PSAUX Alarm(3) LOW Threshold is %0d.%03d Volts. \r\n\r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP4_UPPER, XSysMonPsu_VoltageToRaw(VCC_PSINTFP_DDR_504Data + 0.2), XSYSMON_PS);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP4_UPPER, XSYSMON_PS);//Read the VCC_PSDDR_504 Alarm Threshold registers.
	MaxData	= XSysMonPsu_RawToVoltage(Data);
	printf("\r\nVCC_PSINTFP_DDR_504 Alarm(4) HIGH Threshold is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP4_LOWER, XSysMonPsu_VoltageToRaw(VCC_PSINTFP_DDR_504Data - 0.4), XSYSMON_PS);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP4_LOWER, XSYSMON_PS);
	MinData	= XSysMonPsu_RawToVoltage(Data);
	printf("VCC_PSINTFP_DDR_504 Alarm(4) LOW Threshold is %0d.%03d Volts. \r\n\r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP5_UPPER, XSysMonPsu_VoltageToRaw(VCC_PSIO3_503Data + 0.2), XSYSMON_PS);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP5_UPPER, XSYSMON_PS);//Read the VCC_PSIO3_503 Alarm Threshold registers.
	MaxData	= XSysMonPsu_RawToVoltage(Data);
	printf("\r\nVCC_PSIO3_503 Alarm(5) HIGH Threshold is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP5_LOWER, XSysMonPsu_VoltageToRaw(VCC_PSIO3_503Data - 0.4), XSYSMON_PS);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP5_LOWER, XSYSMON_PS);
	MinData	= XSysMonPsu_RawToVoltage(Data);
	printf("VCC_PSIO3_503 Alarm(5) LOW Threshold is %0d.%03d Volts. \r\n\r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP6_UPPER, XSysMonPsu_VoltageToRaw(VCC_PSIO0_500Data - 0.2), XSYSMON_PS);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP6_UPPER, XSYSMON_PS);//Read the VCC_PSIO0_500 Alarm Threshold registers.
	MaxData	= XSysMonPsu_RawToVoltage(Data);
	printf("\r\nVCC_PSIO0_500 Alarm(6) HIGH Threshold is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP6_LOWER, XSysMonPsu_VoltageToRaw(VCC_PSIO0_500Data - 0.4), XSYSMON_PS);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP6_LOWER, XSYSMON_PS);
	MinData	= XSysMonPsu_RawToVoltage(Data);
	printf("VCC_PSIO0_500 Alarm(6) LOW Threshold is %0d.%03d Volts. \r\n\r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP7_UPPER, XSysMonPsu_VoltageToRaw(VCC_PSIO1_501Data + 0.2), XSYSMON_PS);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP7_UPPER, XSYSMON_PS);//Read the VCC_PSIO1_501 Alarm Threshold registers.
	MaxData	= XSysMonPsu_RawToVoltage(Data);
	printf("\r\nVCC_PSIO1_501 Alarm(7) HIGH Threshold is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP7_LOWER, XSysMonPsu_VoltageToRaw(VCC_PSIO1_501Data - 0.4), XSYSMON_PS);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP7_LOWER, XSYSMON_PS);
	MinData	= XSysMonPsu_RawToVoltage(Data);
	printf("VCC_PSIO1_501 Alarm(7) LOW Threshold is %0d.%03d Volts. \r\n\r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP8_UPPER, XSysMonPsu_VoltageToRaw(VCC_PSIO2_502Data + 0.2), XSYSMON_PS);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP8_UPPER, XSYSMON_PS);//Read the VCC_PSIO2_502 Alarm Threshold registers.
	MaxData	= XSysMonPsu_RawToVoltage(Data);
	printf("\r\nVCC_PSIO2_502 Alarm(8) HIGH Threshold is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP8_LOWER, XSysMonPsu_VoltageToRaw(VCC_PSIO2_502Data - 0.4), XSYSMON_PS);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP8_LOWER, XSYSMON_PS);
	MinData	= XSysMonPsu_RawToVoltage(Data);
	printf("VCC_PSIO2_502 Alarm(8) LOW Threshold is %0d.%03d Volts. \r\n\r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP9_UPPER, XSysMonPsu_VoltageToRaw(PS_MGTRAVCCData - 0.2), XSYSMON_PS);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP9_UPPER, XSYSMON_PS);//Read the PS_MGTRAVCC Alarm Threshold registers.
	MaxData	= XSysMonPsu_RawToVoltage(Data);
	printf("\r\nPS_MGTRAVCC Alarm(9) HIGH Threshold is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP9_LOWER, XSysMonPsu_VoltageToRaw(PS_MGTRAVCCData - 0.4), XSYSMON_PS);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP9_LOWER, XSYSMON_PS);
	MinData	= XSysMonPsu_RawToVoltage(Data);
	printf("PS_MGTRAVCC Alarm(9) LOW Threshold is %0d.%03d Volts. \r\n\r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP10_UPPER, XSysMonPsu_VoltageToRaw(PS_MGTRAVTTData + 0.2), XSYSMON_PS);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP10_UPPER, XSYSMON_PS);//Read the PS_MGTRAVTT Alarm Threshold registers.
	MaxData	= XSysMonPsu_RawToVoltage(Data);
	printf("\r\nPS_MGTRAVTT Alarm(10) HIGH Threshold is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP10_LOWER, XSysMonPsu_VoltageToRaw(PS_MGTRAVTTData - 0.4), XSYSMON_PS);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP10_LOWER, XSYSMON_PS);
	MinData	= XSysMonPsu_RawToVoltage(Data);
	printf("PS_MGTRAVTT Alarm(10) LOW Threshold is %0d.%03d Volts. \r\n\r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));

	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_VCCAMS_UPPER, XSysMonPsu_VoltageToRaw(VCCAMSData + 0.2), XSYSMON_PS);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_VCCAMS_UPPER, XSYSMON_PS);//Read the VCCAMS Alarm Threshold registers.
	MaxData	= XSysMonPsu_RawToVoltage(Data);
	printf("\r\nVCCAMS Alarm(11) HIGH Threshold is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_VCCAMS_LOWER, XSysMonPsu_VoltageToRaw(VCCAMSData - 0.4), XSYSMON_PS);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_VCCAMS_LOWER, XSYSMON_PS);
	MinData	= XSysMonPsu_RawToVoltage(Data);
	printf("VCCAMS Alarm(11) LOW Threshold is %0d.%03d Volts. \r\n\r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));



	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_TEMP_RMTE_UPPER, XSysMonPsu_TemperatureToRaw_OnChip(TempFPData + 10), XSYSMON_PS);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_TEMP_RMTE_UPPER, XSYSMON_PS);//Read the TempFP Temperature Alarm Threshold registers
	MaxData	= XSysMonPsu_RawToTemperature_OnChip(Data);
	printf("\r\nFP Temperature Alarm(12) HIGH Threshold is %0d.%03d Centigrade. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_TEMP_RMTE_LOWER, XSysMonPsu_TemperatureToRaw_OnChip(TempFPData - 20), XSYSMON_PS);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_TEMP_RMTE_LOWER, XSYSMON_PS);
	MinData = XSysMonPsu_RawToTemperature_OnChip(Data);
	printf("FP Temperature Alarm(12) LOW Threshold is %0d.%03d Centigrade. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	//Enable Alarm 0 for on-chip temperature , Alarm 1 for on-chip VCCINT and Alarm 3 for on-chip VCCAUX in the Configuration Register 1.
	XSysMonPsu_SetAlarmEnables(SysMonInstPtr, (XSM_CFR_ALM_TEMP_MASK | XSM_CFR_ALM_SUPPLY1_MASK | XSM_CFR_ALM_SUPPLY2_MASK |XSM_CFR_ALM_SUPPLY3_MASK | XSM_CFR_ALM_SUPPLY4_MASK | XSM_CFR_ALM_SUPPLY5_MASK |
			XSM_CFR_ALM_SUPPLY6_MASK | XSM_CFR_ALM_SUPPLY8_MASK | XSM_CFR_ALM_SUPPLY9_MASK | XSM_CFR_ALM_SUPPLY10_MASK | XSM_CFR_ALM_SUPPLY11_MASK | XSM_CFR_ALM_SUPPLY12_MASK | XSM_CFR_ALM_SUPPLY13_MASK), XSYSMON_PS);
	// Enable Alarm 0 interrupt for on-chip temperature, Alarm 1 interrupt for on-chip VCCINT and Alarm 3 interrupt for on-chip VCCAUX.
	XSysMonPsu_IntrEnable(SysMonInstPtr,XSYSMONPSU_IER_0_PS_ALM_0_MASK | XSYSMONPSU_IER_0_PS_ALM_1_MASK | XSYSMONPSU_IER_0_PS_ALM_2_MASK | XSYSMONPSU_IER_0_PS_ALM_3_MASK | XSYSMONPSU_IER_0_PS_ALM_4_MASK |
			XSYSMONPSU_IER_0_PS_ALM_5_MASK | XSYSMONPSU_IER_0_PS_ALM_6_MASK | XSYSMONPSU_IER_0_PS_ALM_7_MASK | XSYSMONPSU_IER_0_PS_ALM_8_MASK | XSYSMONPSU_IER_0_PS_ALM_9_MASK | XSYSMONPSU_IER_0_PS_ALM_10_MASK |
			XSYSMONPSU_IER_0_PS_ALM_11_MASK | XSYSMONPSU_IER_0_PS_ALM_12_MASK | XSYSMONPSU_IER_0_PS_ALM_13_MASK | XSYSMONPSU_IER_0_PS_ALM_14_MASK | XSYSMONPSU_IER_0_PS_ALM_15_MASK);

	SysMonPsuInterruptHandler(SysMonInstPtr);//interrupt process function


	while (1) // Wait until an Alarm 0 or Alarm 1 or Alarm 3 interrupt occurs.
	{
		if (TempLPIntrActive == TRUE)
		{
			//Alarm 0 - LP Temperature alarm interrupt has occurred.The required processing should be put here.
			printf("\r\nAlarm 0 - LP Temperature alarm has occurred \r\n");
			break;
		}

		if (VCC_PSINTLPIntr == TRUE)
		{
			//Alarm 1 - VCC_PSINTLP alarm interrupt has occurred.The required processing should be put here.
			printf("\r\nAlarm 1 - PS VCCINTLP(SUPPLY1) alarm has occurred \r\n");
			break;
		}

		if (VCCINTFPIntr == TRUE)
		{
			//Alarm 2 - VCCINT alarm interrupt has occurred.The required processing should be put here.
			printf("\r\nAlarm 2 - VCCINTFP(SUPPLY2) alarm has occurred \r\n");
			break;
		}

		if (VCC_PSAUXIntr == TRUE)
		{
			//Alarm 3 - VCC_PSAUX alarm interrupt has occurred.The required processing should be put here.
			printf("\r\nAlarm 3 - VCC_PSAUX(SUPPLY3) alarm has occurred \r\n");
			break;
		}
		if (VCC_PSINTFP_DDR_504Intr == TRUE)
		{
			//Alarm 4 - VCC_PSDDR_504 alarm interrupt has occurred.The required processing should be put here.
			printf("\r\nAlarm 4 - VCC_PSINTFP_DDR_504(SUPPLY4) alarm has occurred \r\n");
			break;
		}
		if (VCC_PSIO3_503Intr == TRUE)
		{
			//Alarm 5 - VCC_PSIO3_503 alarm interrupt has occurred.The required processing should be put here.
			printf("\r\nAlarm 5 - VCC_PSIO3_503(SUPPLY5) alarm has occurred \r\n");
			break;
		}
		if (VCC_PSIO0_500Intr == TRUE)
		{
			//Alarm 6 - VCC_PSIO0_500 alarm interrupt has occurred.The required processing should be put here.
			printf("\r\nAlarm 6 - VCC_PSIO0_500(SUPPLY6) alarm has occurred \r\n");
			break;
		}
		if (VCC_PSIO1_501Intr == TRUE)
		{
			//Alarm 7 - VCC_PSIO1_501 alarm interrupt has occurred.The required processing should be put here.
			printf("\r\nAlarm 7 - VCC_PSIO1_501(SUPPLY7) alarm has occurred \r\n");
			break;
		}
		if (VCC_PSIO2_502Intr == TRUE)
		{
			//Alarm 8 - VCC_PSIO2_502 alarm interrupt has occurred.The required processing should be put here.
			printf("\r\nAlarm 8 - VCC_PSIO2_502(SUPPLY8) alarm has occurred \r\n");
			break;
		}
		if (PS_MGTRAVCCIntr == TRUE)
		{
			//Alarm 9 - PS_MGTRAVCC alarm interrupt has occurred.The required processing should be put here.
			printf("\r\nAlarm 9 - PS_MGTRAVCC(SUPPLY9) alarm has occurred \r\n");
			break;
		}

		if (PS_MGTRAVTTIntr == TRUE)
		{
			//Alarm 10 - PS_MGTRAVTT alarm interrupt has occurred.The required processing should be put here.
			printf("\r\nAlarm 10 - PS_MGTRAVTT(SUPPLY10) alarm has occurred \r\n");
			break;
		}
		if (VCCAMSIntr == TRUE)
		{
			//Alarm 11 - VCCAMS alarm interrupt has occurred.The required processing should be put here.
			printf("\r\nAlarm 11 - VCCAMS alarm has occurred \r\n");
			break;
		}

		if (TempFPIntrActive == TRUE)
		{
			//Alarm 12 - FP Temperature alarm interrupt has occurred.The required processing should be put here.
			printf("\r\nAlarm 12 - FP Temperature alarm has occurred \r\n");
			break;
		}
	}

	printf("\r\nExiting the PS_SysMon Interrupt Example. \r\n");

	return XST_SUCCESS;
}


int PL_SysMonPsuIntrExample(XScuGic* XScuGicInstPtr, XSysMonPsu* SysMonInstPtr, u16 SysMonDeviceId, u16 SysMonIntrId)
{
	//PL_Temp,PL_VCCINT,PL_VCCAUX,PL_VP_VN,PL_VREFP,PL_VREFN,PL_VCCBRAM,PL_VCC_PSINTLP,PL_VCC_PSINTFP,PL_VCC_PSAUX,PL_VAUXP1,PL_VUser0,PL_VCCADC
	//AMSVCC_PSPLL,AMSVCC_PSBATT,AMSVCCINT,AMSVCCBRAM,AMSVCCAUX,AMSVCC_PSDDR_PLL,AMSVCC_DDRPHY_REF,AMSVCC_PSINTFO_DDR
	int Status;
	XSysMonPsu_Config *ConfigPtr;
	u64 IntrStatus;
	u32 Data;
	u32 PL_TempRawData, PL_VCCINTRawData, PL_VCCAUXRawData, PL_VP_VNRawData, PL_VREFPRawData, PL_VREFNRawData, PL_VCCBRAMRawData, PL_VCC_PSINTLPRawData, \
	PL_VCC_PSINTFPRawData, PL_VCC_PSAUXRawData, PL_VAUXP1RawData, PL_VUser0RawData, PL_VUser1RawData, PL_VUser2RawData, PL_VUser3RawData, PL_VCCADCRawData;
	float PL_TempData, PL_VCCINTData, PL_VCCAUXData, PL_VP_VNData, PL_VREFPData, PL_VREFNData, PL_VCCBRAMData, PL_VCC_PSINTLPData, PL_VCC_PSINTFPData, \
	PL_VCC_PSAUXData, PL_VAUXP1Data, PL_VUser0Data, PL_VUser1Data, PL_VUser2Data, PL_VUser3Data, PL_VCCADCData, MaxData, MinData;

	//u32 AMSVCC_PSPLLRawData, AMSVCC_PSBATTRawData, AMSVCCINTRawData, AMSVCCBRAMRawData, AMSVCCAUXRawData, AMSVCC_PSDDR_PLLRawData, AMSVCC_DDRPHY_REFRawData, AMSVCC_PSINTFO_DDRRawData;
	//float AMSVCC_PSPLLData, AMSVCC_PSBATTData, AMSVCCINTData, AMSVCCBRAMData, AMSVCCAUXData, AMSVCC_PSDDR_PLLData, AMSVCC_DDRPHY_REFData, AMSVCC_PSINTFO_DDRData;

	printf("Entering the PL_SysMonPsu Interrupt Example. \r\n");

	ConfigPtr = XSysMonPsu_LookupConfig(SysMonDeviceId);//Initialize the SysMon driver
	if (ConfigPtr == NULL)
	{
		return XST_FAILURE;
	}

	XSysMonPsu_CfgInitialize(SysMonInstPtr, ConfigPtr, ConfigPtr->BaseAddress);

	Status = XSysMonPsu_SelfTest(SysMonInstPtr);//Self Test the System Monitor device
	if (Status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}

	XSysMonPsu_SetSequencerMode(SysMonInstPtr, XSM_SEQ_MODE_SAFE, XSYSMON_PL);//Disable the Channel Sequencer before configuring the Sequence registers
	XSysMonPsu_SetAvg(SysMonInstPtr, XSM_AVG_16_SAMPLES, XSYSMON_PL);//Setup the Averaging to be done for the channels in the Configuration 0 register as 16 samples

	Status = XSysMonPsu_SetSeqInputMode(SysMonInstPtr, XSYSMONPSU_SEQ_CH1_VAUX00_MASK << 16, XSYSMON_PL);
	if (Status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}

	Status = XSysMonPsu_SetSeqAcqTime(SysMonInstPtr, (XSYSMONPSU_SEQ_CH1_VAUX0F_MASK | XSYSMONPSU_SEQ_CH1_VAUX00_MASK) << 16, XSYSMON_PL);
	if (Status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}


	//Enable the averaging on the following channels in the Sequencer registers:On-chip Temperature, Remote temperature,VCC_PSINTLP,VCC_PSINTFP,VCC_PSAUX,
	//VCC_PSDDR_504,VCCAMS,VCC_PSIO3_503,VCC_PSIO0_500,VCC_PSIO1_501,VCC_PSIO2_502,PS_MGTRAVCC,PS_MGTRAVTT
	Status =  XSysMonPsu_SetSeqAvgEnables(SysMonInstPtr, XSYSMONPSU_SEQ_CH0_TEMP_MASK |
	XSYSMONPSU_SEQ_CH0_SUP1_MASK | XSYSMONPSU_SEQ_CH0_SUP2_MASK |
	XSYSMONPSU_SEQ_CH0_SUP3_MASK | XSYSMONPSU_SEQ_CH0_SUP4_MASK |
	XSYSMONPSU_SEQ_CH0_SUP5_MASK | XSYSMONPSU_SEQ_CH0_SUP6_MASK | XSYSMONPSU_SEQ_CH0_VP_VN_MASK | XSYSMONPSU_SEQ_CH0_VREFP_MASK | XSYSMONPSU_SEQ_CH0_VREFN_MASK |
	(XSYSMONPSU_SEQ_CH2_SUP7_MASK | XSYSMONPSU_SEQ_CH2_SUP8_MASK |
	XSYSMONPSU_SEQ_CH2_SUP9_MASK | XSYSMONPSU_SEQ_CH2_SUP10_MASK |(u64)(XSYSMONPSU_SEQ_CH2_VCCAMS_MASK)) << 32, XSYSMON_PL);

	if (Status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}


	//Enable the averaging on the following channels in the Sequencer registers:On-chip Temperature, Remote temperature,VCC_PSINTLP,VCC_PSINTFP,VCC_PSAUX,
	//VCC_PSDDR_504,VCCAMS,VCC_PSIO3_503,VCC_PSIO0_500,VCC_PSIO1_501,VCC_PSIO2_502,PS_MGTRAVCC,PS_MGTRAVTT
	Status =  XSysMonPsu_SetSeqChEnables(SysMonInstPtr, XSYSMONPSU_SEQ_CH0_TEMP_MASK |
	XSYSMONPSU_SEQ_CH0_SUP1_MASK | XSYSMONPSU_SEQ_CH0_SUP2_MASK |
	XSYSMONPSU_SEQ_CH0_SUP3_MASK | XSYSMONPSU_SEQ_CH0_SUP4_MASK |
	XSYSMONPSU_SEQ_CH0_SUP5_MASK | XSYSMONPSU_SEQ_CH0_SUP6_MASK | XSYSMONPSU_SEQ_CH0_VP_VN_MASK | XSYSMONPSU_SEQ_CH0_VREFP_MASK | XSYSMONPSU_SEQ_CH0_VREFN_MASK |
	(XSYSMONPSU_SEQ_CH2_SUP7_MASK | XSYSMONPSU_SEQ_CH2_SUP8_MASK |
	XSYSMONPSU_SEQ_CH2_SUP9_MASK | XSYSMONPSU_SEQ_CH2_SUP10_MASK |(u64)(XSYSMONPSU_SEQ_CH2_VCCAMS_MASK)) << 32, XSYSMON_PL);
	if (Status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}

	IntrStatus = XSysMonPsu_IntrGetStatus(SysMonInstPtr);//Clear any bits set in the Interrupt Status Register
	XSysMonPsu_IntrClear(SysMonInstPtr, IntrStatus);

	XSysMonPsu_SetSequencerMode(SysMonInstPtr, XSM_SEQ_MODE_CONTINPASS, XSYSMON_PL);//Enable the Channel Sequencer in continuous sequencer cycling mode.

	// Wait till the End of Sequence occurs
	while ((XSysMonPsu_IntrGetStatus(SysMonInstPtr) & ((u64)XSYSMONPSU_ISR_1_EOS_MASK << 32)) != ((u64)XSYSMONPSU_ISR_1_EOS_MASK << 32));

	//Read the ADC converted Data from the data registers for on-chip temperature, on-chip VCCINT voltage and on-chip VCCAUX voltage.
	PL_TempRawData = XSysMonPsu_GetAdcData(SysMonInstPtr, XSM_CH_TEMP, XSYSMON_PL);
	PL_TempData = XSysMonPsu_RawToTemperature_OnChip(PL_TempRawData);//Convert the Raw Data to Degrees Centigrade and Voltage
	printf("\r\nThe Current PL Temperature is %0d.%03d Centigrade.\r\n", (int)(PL_TempData), SysMonPsuFractionToInt(PL_TempData));
	PL_TempRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MAX_TEMP, XSYSMON_PL);
	MaxData	= XSysMonPsu_RawToTemperature_OnChip(PL_TempRawData);
	printf("The Maximum PL Temperature is %0d.%03d Centigrade. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	PL_TempRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MIN_TEMP, XSYSMON_PL);
	MinData	= XSysMonPsu_RawToTemperature_OnChip(PL_TempRawData);
	printf("The Minimum PL Temperature is %0d.%03d Centigrade. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	PL_VCCINTRawData = XSysMonPsu_GetAdcData(SysMonInstPtr, XSM_CH_SUPPLY1, XSYSMON_PL);
	PL_VCCINTData	= XSysMonPsu_RawToVoltage(PL_VCCINTRawData);
	printf("\r\nThe Current PL_VCCINT is %0d.%03d Volts. \r\n", (int)(PL_VCCINTData), SysMonPsuFractionToInt(PL_VCCINTData));
	PL_VCCINTRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MAX_SUPPLY1, XSYSMON_PL);
	MaxData	= XSysMonPsu_RawToVoltage(PL_VCCINTRawData);
	printf("The Maximum PL_VCCINT is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	PL_VCCINTRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MIN_SUPPLY1, XSYSMON_PL);
	MinData	= XSysMonPsu_RawToVoltage(PL_VCCINTRawData);
	printf("The Minimum PL_VCCINT is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	PL_VCCAUXRawData = XSysMonPsu_GetAdcData(SysMonInstPtr, XSM_CH_SUPPLY2, XSYSMON_PL);
	PL_VCCAUXData	= XSysMonPsu_RawToVoltage(PL_VCCAUXRawData);
	printf("\r\nThe Current PL_VCCAUX is %0d.%03d Volts. \r\n", (int)(PL_VCCAUXData), SysMonPsuFractionToInt(PL_VCCAUXData));
	PL_VCCAUXRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MAX_SUPPLY2, XSYSMON_PL);
	MaxData	= XSysMonPsu_RawToVoltage(PL_VCCAUXRawData);
	printf("The Maximum PL_VCCAUX is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	PL_VCCAUXRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MIN_SUPPLY2, XSYSMON_PL);
	MinData	= XSysMonPsu_RawToVoltage(PL_VCCAUXRawData);
	printf("The Minimum PL_VCCAUX is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	PL_VP_VNRawData = XSysMonPsu_GetAdcData(SysMonInstPtr, XSM_CH_VPVN, XSYSMON_PL);
	PL_VP_VNData	= XSysMonPsu_RawToVoltage(PL_VP_VNRawData);
	printf("\r\nThe Current PL_VP_VN is %0d.%03d Volts. \r\n", (int)(PL_VP_VNData), SysMonPsuFractionToInt(PL_VP_VNData));
	/*PL_VP_VNRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_CH_VPVN, XSYSMON_PL);
	MaxData	= XSysMonPsu_RawToVoltage(PL_VP_VNRawData);
	printf("The Maximum PL_VP_VN is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	PL_VP_VNRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_CH_VPVN, XSYSMON_PL);
	MinData	= XSysMonPsu_RawToVoltage(PL_VP_VNRawData);
	printf("The Minimum PL_VP_VN is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));*/


	PL_VREFPRawData = XSysMonPsu_GetAdcData(SysMonInstPtr, XSM_CH_VREFP, XSYSMON_PL);
	PL_VREFPData	= XSysMonPsu_RawToVoltage(PL_VREFPRawData);
	printf("\r\nThe Current PL_VREFP is %0d.%03d Volts. \r\n", (int)(PL_VREFPData), SysMonPsuFractionToInt(PL_VREFPData));
	/*PL_VREFPRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_CH_VREFP, XSYSMON_PL);
	MaxData	= XSysMonPsu_RawToVoltage(PL_VREFPRawData);
	printf("The Maximum PL_VREFP is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	PL_VREFPRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_CH_VREFP, XSYSMON_PL);
	MinData	= XSysMonPsu_RawToVoltage(PL_VREFPRawData);
	printf("The Minimum PL_VREFP is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));*/


	PL_VREFNRawData = XSysMonPsu_GetAdcData(SysMonInstPtr, XSM_CH_VREFN, XSYSMON_PL);
	PL_VREFNData	= XSysMonPsu_RawToVoltage(PL_VREFNRawData);
	printf("\r\nThe Current PL_VREFN is %0d.%03d Volts. \r\n", (int)(PL_VREFNData), SysMonPsuFractionToInt(PL_VREFNData));
	/*PL_VREFNRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_CH_VREFN, XSYSMON_PL);
	MaxData	= XSysMonPsu_RawToVoltage(PL_VREFNRawData);
	printf("The Maximum PL_VREFN is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	PL_VREFNRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_CH_VREFN, XSYSMON_PL);
	MinData	= XSysMonPsu_RawToVoltage(PL_VREFNRawData);
	printf("The Minimum PL_VREFN is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));*/


	PL_VCCBRAMRawData = XSysMonPsu_GetAdcData(SysMonInstPtr, XSM_CH_SUPPLY3, XSYSMON_PL);
	PL_VCCBRAMData	= XSysMonPsu_RawToVoltage(PL_VCCBRAMRawData);
	printf("\r\nThe Current PL_VCCBRAM is %0d.%03d Volts. \r\n", (int)(PL_VCCBRAMData), SysMonPsuFractionToInt(PL_VCCBRAMData));
	PL_VCCBRAMRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MAX_SUPPLY3, XSYSMON_PL);
	MaxData	= XSysMonPsu_RawToVoltage(PL_VCCBRAMRawData);
	printf("The Maximum PL_VCCBRAM is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	PL_VCCBRAMRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MIN_SUPPLY3, XSYSMON_PL);
	MinData	= XSysMonPsu_RawToVoltage(PL_VCCBRAMRawData);
	printf("The Minimum PL_VCCBRAM is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	PL_VCC_PSINTLPRawData = XSysMonPsu_GetAdcData(SysMonInstPtr, XSM_CH_SUPPLY4, XSYSMON_PL);
	PL_VCC_PSINTLPData	= XSysMonPsu_RawToVoltage(PL_VCC_PSINTLPRawData);
	printf("\r\nThe Current PL_VCC_PSINTLP is %0d.%03d Volts. \r\n", (int)(PL_VCC_PSINTLPData), SysMonPsuFractionToInt(PL_VCC_PSINTLPData));
	PL_VCC_PSINTLPRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MAX_SUPPLY4, XSYSMON_PL);
	MaxData	= XSysMonPsu_RawToVoltage(PL_VCC_PSINTLPRawData);
	printf("The Maximum PL_VCC_PSINTLP is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	PL_VCC_PSINTLPRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MIN_SUPPLY4, XSYSMON_PL);
	MinData	= XSysMonPsu_RawToVoltage(PL_VCC_PSINTLPRawData);
	printf("The Minimum PL_VCC_PSINTLP is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	PL_VCC_PSINTFPRawData = XSysMonPsu_GetAdcData(SysMonInstPtr, XSM_CH_SUPPLY5, XSYSMON_PL);
	PL_VCC_PSINTFPData	= XSysMonPsu_RawToVoltage(PL_VCC_PSINTFPRawData);
	printf("\r\nThe Current PL_VCC_PSINTFP is %0d.%03d Volts. \r\n", (int)(PL_VCC_PSINTFPData), SysMonPsuFractionToInt(PL_VCC_PSINTFPData));
	PL_VCC_PSINTFPRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MAX_SUPPLY5, XSYSMON_PL);
	MaxData	= XSysMonPsu_RawToVoltage(PL_VCC_PSINTFPRawData);
	printf("The Maximum PL_VCC_PSINTFP is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	PL_VCC_PSINTFPRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MIN_SUPPLY5, XSYSMON_PL);
	MinData	= XSysMonPsu_RawToVoltage(PL_VCC_PSINTFPRawData);
	printf("The Minimum PL_VCC_PSINTFP is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	PL_VCC_PSAUXRawData = XSysMonPsu_GetAdcData(SysMonInstPtr, XSM_CH_SUPPLY6, XSYSMON_PL);
	PL_VCC_PSAUXData	= XSysMonPsu_RawToVoltage(PL_VCC_PSAUXRawData);
	printf("\r\nThe Current PL_VCC_PSAUX is %0d.%03d Volts. \r\n", (int)(PL_VCC_PSAUXData), SysMonPsuFractionToInt(PL_VCC_PSAUXData));
	PL_VCC_PSAUXRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MAX_SUPPLY6, XSYSMON_PL);
	MaxData	= XSysMonPsu_RawToVoltage(PL_VCC_PSAUXRawData);
	printf("The Maximum PL_VCC_PSAUX is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	PL_VCC_PSAUXRawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MIN_SUPPLY6, XSYSMON_PL);
	MinData	= XSysMonPsu_RawToVoltage(PL_VCC_PSAUXRawData);
	printf("The Minimum PL_VCC_PSAUX is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	PL_VUser0RawData = XSysMonPsu_GetAdcData(SysMonInstPtr, XSM_CH_SUPPLY7, XSYSMON_PL);
	PL_VUser0Data	= XSysMonPsu_RawToVoltage(PL_VUser0RawData);
	printf("\r\nThe Current PL_VUser0 is %0d.%03d Volts. \r\n", (int)(PL_VUser0Data), SysMonPsuFractionToInt(PL_VUser0Data));
	PL_VUser0RawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MAX_SUPPLY7, XSYSMON_PL);
	MaxData	= XSysMonPsu_RawToVoltage(PL_VUser0RawData);
	printf("The Maximum PL_VUser0 is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	PL_VUser0RawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MIN_SUPPLY7, XSYSMON_PL);
	MinData	= XSysMonPsu_RawToVoltage(PL_VUser0RawData);
	printf("The Minimum PL_VUser0 is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	PL_VUser1RawData = XSysMonPsu_GetAdcData(SysMonInstPtr, XSM_CH_SUPPLY8, XSYSMON_PL);
	PL_VUser1Data	= XSysMonPsu_RawToVoltage(PL_VUser1RawData);
	printf("\r\nThe Current PL_VUser1 is %0d.%03d Volts. \r\n", (int)(PL_VUser1Data), SysMonPsuFractionToInt(PL_VUser1Data));
	PL_VUser1RawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MAX_SUPPLY8, XSYSMON_PL);
	MaxData	= XSysMonPsu_RawToVoltage(PL_VUser1RawData);
	printf("The Maximum PL_VUser1 is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	PL_VUser1RawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MIN_SUPPLY8, XSYSMON_PL);
	MinData	= XSysMonPsu_RawToVoltage(PL_VUser1RawData);
	printf("The Minimum PL_VUser1 is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	PL_VUser2RawData = XSysMonPsu_GetAdcData(SysMonInstPtr, XSM_CH_SUPPLY9, XSYSMON_PL);
	PL_VUser2Data	= XSysMonPsu_RawToVoltage(PL_VUser2RawData);
	printf("\r\nThe Current PL_VUser2 is %0d.%03d Volts. \r\n", (int)(PL_VUser2Data), SysMonPsuFractionToInt(PL_VUser2Data));
	PL_VUser2RawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MAX_SUPPLY9, XSYSMON_PL);
	MaxData	= XSysMonPsu_RawToVoltage(PL_VUser2RawData);
	printf("The Maximum PL_VUser2 is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	PL_VUser2RawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MIN_SUPPLY9, XSYSMON_PL);
	MinData	= XSysMonPsu_RawToVoltage(PL_VUser2RawData);
	printf("The Minimum PL_VUser2 is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	PL_VUser3RawData = XSysMonPsu_GetAdcData(SysMonInstPtr, XSM_CH_SUPPLY10, XSYSMON_PL);
	PL_VUser3Data	= XSysMonPsu_RawToVoltage(PL_VUser3RawData);
	printf("\r\nThe Current PL_VUser3 is %0d.%03d Volts. \r\n", (int)(PL_VUser3Data), SysMonPsuFractionToInt(PL_VUser3Data));
	PL_VUser3RawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MAX_SUPPLY10, XSYSMON_PL);
	MaxData	= XSysMonPsu_RawToVoltage(PL_VUser3RawData);
	printf("The Maximum PL_VUser3 is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	PL_VUser3RawData = XSysMonPsu_GetMinMaxMeasurement(SysMonInstPtr, XSM_MIN_SUPPLY10, XSYSMON_PL);
	MinData	= XSysMonPsu_RawToVoltage(PL_VUser3RawData);
	printf("The Minimum PL_VUser3 is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));

	XSysMonPsu_SetAlarmEnables(SysMonInstPtr, 0, XSYSMON_PL);//Disable all the alarms in the Configuration Register 1.

	Status = SysMonPsuSetupInterruptSystem(XScuGicInstPtr, SysMonInstPtr, SysMonIntrId);//Set up the AMS interrupt system,INTR_ID = XPAR_XSYSMONPSU_INTR = XPS_AMS_INT_ID = (56U + 32U)
	if (Status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}

	IntrStatus = XSysMonPsu_IntrGetStatus(SysMonInstPtr);//Clear any bits set in the Interrupt Status Register.
	XSysMonPsu_IntrClear(SysMonInstPtr, IntrStatus);


	 // Set up Alarm threshold registers for On-chip Temperature High/Low limit, VCCINT High/Low limit,VCCAUX High/Low limit, so that the Alarms occur.
	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_TEMP_UPPER, XSysMonPsu_TemperatureToRaw_OnChip(PL_TempData + 10), XSYSMON_PL);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_TEMP_UPPER, XSYSMON_PL);//Read the PL Temperature Alarm Threshold registers
	MaxData	= XSysMonPsu_RawToTemperature_OnChip(Data);
	printf("\r\nLP Temperature Alarm(0) HIGH Threshold is %0d.%03d Centigrade. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_TEMP_LOWER, XSysMonPsu_TemperatureToRaw_OnChip(PL_TempData - 20), XSYSMON_PL);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_TEMP_LOWER, XSYSMON_PL);
	MinData = XSysMonPsu_RawToTemperature_OnChip(Data);
	printf("LP Temperature Alarm(0) LOW Threshold is %0d.%03d Centigrade. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP1_UPPER, XSysMonPsu_VoltageToRaw(PL_VCCINTData + 0.2), XSYSMON_PL);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP1_UPPER, XSYSMON_PL);//Read the PL_VCCINT Alarm Threshold registers.
	MaxData	= XSysMonPsu_RawToVoltage(Data);
	printf("\r\nPL_VCCINT Alarm(1) HIGH Threshold is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
    XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP1_LOWER, XSysMonPsu_VoltageToRaw(PL_VCCINTData - 0.4), XSYSMON_PL);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP1_LOWER, XSYSMON_PL);
	MinData	= XSysMonPsu_RawToVoltage(Data);
	printf("PL_VCCINT Alarm(1) LOW Threshold is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP2_UPPER, XSysMonPsu_VoltageToRaw(PL_VCCAUXData + 0.2), XSYSMON_PL);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP2_UPPER, XSYSMON_PL);//Read the PL_VCCAUX Alarm Threshold registers.
	MaxData	= XSysMonPsu_RawToVoltage(Data);
	printf("\r\nPL_VCCAUX Alarm(2) HIGH Threshold is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
    XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP2_LOWER, XSysMonPsu_VoltageToRaw(PL_VCCAUXData - 0.4), XSYSMON_PL);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP2_LOWER, XSYSMON_PL);
	MinData	= XSysMonPsu_RawToVoltage(Data);
	printf("PL_VCCAUX Alarm(2) LOW Threshold is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP3_UPPER, XSysMonPsu_VoltageToRaw(PL_VCCBRAMData + 0.2), XSYSMON_PL);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP3_UPPER, XSYSMON_PL);//Read the PL_VCCBRAM Alarm Threshold registers.
	MaxData	= XSysMonPsu_RawToVoltage(Data);
	printf("\r\nPL_VCCBRAM Alarm(3) HIGH Threshold is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
    XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP3_LOWER, XSysMonPsu_VoltageToRaw(PL_VCCBRAMData - 0.4), XSYSMON_PL);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP3_LOWER, XSYSMON_PL);
	MinData	= XSysMonPsu_RawToVoltage(Data);
	printf("PL_VCCBRAM Alarm(3) LOW Threshold is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP4_UPPER, XSysMonPsu_VoltageToRaw(PL_VCC_PSINTLPData + 0.2), XSYSMON_PL);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP4_UPPER, XSYSMON_PL);//Read the PL_VCC_PSINTLP Alarm Threshold registers.
	MaxData	= XSysMonPsu_RawToVoltage(Data);
	printf("\r\nPL_VCC_PSINTLP Alarm(4) HIGH Threshold is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
    XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP4_LOWER, XSysMonPsu_VoltageToRaw(PL_VCC_PSINTLPData - 0.4), XSYSMON_PL);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP4_LOWER, XSYSMON_PL);
	MinData	= XSysMonPsu_RawToVoltage(Data);
	printf("PL_VCC_PSINTLP Alarm(4) LOW Threshold is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP5_UPPER, XSysMonPsu_VoltageToRaw(PL_VCC_PSINTFPData + 0.2), XSYSMON_PL);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP5_UPPER, XSYSMON_PL);//Read the PL_VCC_PSINTFP Alarm Threshold registers.
	MaxData	= XSysMonPsu_RawToVoltage(Data);
	printf("\r\nPL_VCC_PSINTFP Alarm(5) HIGH Threshold is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
    XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP5_LOWER, XSysMonPsu_VoltageToRaw(PL_VCC_PSINTFPData - 0.4), XSYSMON_PL);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP5_LOWER, XSYSMON_PL);
	MinData	= XSysMonPsu_RawToVoltage(Data);
	printf("PL_VCC_PSINTFP Alarm(5) LOW Threshold is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP6_UPPER, XSysMonPsu_VoltageToRaw(PL_VCC_PSAUXData - 0.2), XSYSMON_PL);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP6_UPPER, XSYSMON_PL);//Read the PL_VCC_PSAUX Alarm Threshold registers.
	MaxData	= XSysMonPsu_RawToVoltage(Data);
	printf("\r\nPL_VCC_PSAUX Alarm(6) HIGH Threshold is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
    XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP6_LOWER, XSysMonPsu_VoltageToRaw(PL_VCC_PSAUXData - 0.4), XSYSMON_PL);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP6_LOWER, XSYSMON_PL);
	MinData	= XSysMonPsu_RawToVoltage(Data);
	printf("PL_VCC_PSAUX Alarm(6) LOW Threshold is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP7_UPPER, XSysMonPsu_VoltageToRaw(PL_VUser0Data + 0.2), XSYSMON_PL);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP7_UPPER, XSYSMON_PL);//Read the PL_VUser0 Alarm Threshold registers.
	MaxData	= XSysMonPsu_RawToVoltage(Data);
	printf("\r\nPL_VUser0 Alarm(8) HIGH Threshold is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
    XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP7_LOWER, XSysMonPsu_VoltageToRaw(PL_VUser0Data - 0.4), XSYSMON_PL);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP7_LOWER, XSYSMON_PL);
	MinData	= XSysMonPsu_RawToVoltage(Data);
	printf("PL_VUser0 Alarm(8) LOW Threshold is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP8_UPPER, XSysMonPsu_VoltageToRaw(PL_VUser1Data + 0.2), XSYSMON_PL);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP8_UPPER, XSYSMON_PL);//Read the PL_VUser1 Alarm Threshold registers.
	MaxData	= XSysMonPsu_RawToVoltage(Data);
	printf("\r\nPL_VUser1 Alarm(9) HIGH Threshold is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
    XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP8_LOWER, XSysMonPsu_VoltageToRaw(PL_VUser1Data - 0.4), XSYSMON_PL);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP8_LOWER, XSYSMON_PL);
	MinData	= XSysMonPsu_RawToVoltage(Data);
	printf("PL_VUser1 Alarm(9) LOW Threshold is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP9_UPPER, XSysMonPsu_VoltageToRaw(PL_VUser2Data + 0.2), XSYSMON_PL);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP9_UPPER, XSYSMON_PL);//Read the PL_VUser2 Alarm Threshold registers.
	MaxData	= XSysMonPsu_RawToVoltage(Data);
	printf("\r\nPL_VUser2 Alarm(10) HIGH Threshold is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
    XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP9_LOWER, XSysMonPsu_VoltageToRaw(PL_VUser2Data - 0.4), XSYSMON_PL);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP9_LOWER, XSYSMON_PL);
	MinData	= XSysMonPsu_RawToVoltage(Data);
	printf("PL_VUser2 Alarm(10) LOW Threshold is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP10_UPPER, XSysMonPsu_VoltageToRaw(PL_VUser3Data + 0.2), XSYSMON_PL);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP10_UPPER, XSYSMON_PL);//Read the PL_VUser3 Alarm Threshold registers.
	MaxData	= XSysMonPsu_RawToVoltage(Data);
	printf("\r\nPL_VUser3 Alarm(11) HIGH Threshold is %0d.%03d Volts. \r\n", (int)(MaxData), SysMonPsuFractionToInt(MaxData));
    XSysMonPsu_SetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP10_LOWER, XSysMonPsu_VoltageToRaw(PL_VUser3Data - 0.4), XSYSMON_PL);
	Data = XSysMonPsu_GetAlarmThreshold(SysMonInstPtr, XSM_ATR_SUP10_LOWER, XSYSMON_PL);
	MinData	= XSysMonPsu_RawToVoltage(Data);
	printf("PL_VUser3 Alarm(11) LOW Threshold is %0d.%03d Volts. \r\n", (int)(MinData), SysMonPsuFractionToInt(MinData));


	//Enable Alarm 0 for on-chip temperature , Alarm 1 for on-chip VCCINT and Alarm 3 for on-chip VCCAUX in the Configuration Register 1.
	XSysMonPsu_SetAlarmEnables(SysMonInstPtr, (XSM_CFR_ALM_TEMP_MASK | XSM_CFR_ALM_SUPPLY1_MASK | XSM_CFR_ALM_SUPPLY2_MASK |XSM_CFR_ALM_SUPPLY3_MASK | XSM_CFR_ALM_SUPPLY4_MASK | XSM_CFR_ALM_SUPPLY5_MASK |
			XSM_CFR_ALM_SUPPLY6_MASK | XSM_CFR_ALM_SUPPLY8_MASK | XSM_CFR_ALM_SUPPLY9_MASK | XSM_CFR_ALM_SUPPLY10_MASK | XSM_CFR_ALM_SUPPLY11_MASK | XSM_CFR_ALM_SUPPLY12_MASK | XSM_CFR_ALM_SUPPLY13_MASK), XSYSMON_PL);
	// Enable Alarm 0 interrupt for on-chip temperature, Alarm 1 interrupt for on-chip VCCINT and Alarm 3 interrupt for on-chip VCCAUX.
	XSysMonPsu_IntrEnable(SysMonInstPtr,XSYSMONPSU_IER_0_PS_ALM_0_MASK | XSYSMONPSU_IER_0_PS_ALM_1_MASK | XSYSMONPSU_IER_0_PS_ALM_2_MASK | XSYSMONPSU_IER_0_PS_ALM_3_MASK | XSYSMONPSU_IER_0_PS_ALM_4_MASK |
			XSYSMONPSU_IER_0_PS_ALM_5_MASK | XSYSMONPSU_IER_0_PS_ALM_6_MASK | XSYSMONPSU_IER_0_PS_ALM_7_MASK | XSYSMONPSU_IER_0_PS_ALM_8_MASK | XSYSMONPSU_IER_0_PS_ALM_9_MASK | XSYSMONPSU_IER_0_PS_ALM_10_MASK |
			XSYSMONPSU_IER_0_PS_ALM_11_MASK | XSYSMONPSU_IER_0_PS_ALM_12_MASK | XSYSMONPSU_IER_0_PS_ALM_13_MASK | XSYSMONPSU_IER_0_PS_ALM_14_MASK | XSYSMONPSU_IER_0_PS_ALM_15_MASK);

	SysMonPsuInterruptHandler(SysMonInstPtr);//interrupt process function


	while (1) // Wait until an Alarm 0 or Alarm 1 or Alarm 3 interrupt occurs.
	{
		if (PL_TempActive == TRUE)
		{
			//Alarm 0 - PL Temperature alarm interrupt has occurred.The required processing should be put here.
			printf("\r\nAlarm 0 - PL Temperature alarm has occurred \r\n");
			break;
		}

		if (PL_VCCINTIntr == TRUE)
		{
			//Alarm 1 - PL_VCCINT alarm interrupt has occurred.The required processing should be put here.
			printf("\r\nAlarm 1 - PL_VCCINT(SUPPLY1) alarm has occurred \r\n");
			break;
		}

		if (PL_VCCAUXIntr == TRUE)
		{
			//Alarm 2 - PL_VCCAUX alarm interrupt has occurred.The required processing should be put here.
			printf("\r\nAlarm 2 - PL_VCCAUX(SUPPLY2) alarm has occurred \r\n");
			break;
		}
		if (PL_VCCBRAMIntr == TRUE)
		{
			//Alarm 3 - PL_VCCBRAM alarm interrupt has occurred.The required processing should be put here.
			printf("\r\nAlarm 3 - PL_VCCBRAM alarm has occurred \r\n");
			break;
		}
		if (PL_VREFPIntr == TRUE)
		{
			//Alarm 4 - PL_VREFP alarm interrupt has occurred.The required processing should be put here.
			printf("\r\nAlarm 4 - PL_VREFP alarm has occurred \r\n");
			break;
		}
		if (PL_VREFNIntr == TRUE)
		{
			//Alarm 5 - PL_VREFN alarm interrupt has occurred.The required processing should be put here.
			printf("\r\nAlarm 5 - PL_VREFN alarm has occurred \r\n");
			break;
		}
		if (PL_VP_VNIntr == TRUE)
		{
			//Alarm 6 - PL_VP_VN alarm interrupt has occurred.The required processing should be put here.
			printf("\r\nAlarm 6 - PL_VP_VN alarm has occurred \r\n");
			break;
		}
		if (PL_VCC_PSINTLPIntr == TRUE)
		{
			//Alarm X - PL_VCC_PSINTLP alarm interrupt has occurred.The required processing should be put here.
			printf("\r\nAlarm X - PL_VCC_PSINTLP(SUPPLY4) alarm has occurred \r\n");
			break;
		}
		if (PL_VCC_PSINTFPIntr == TRUE)
		{
			//Alarm X - PL_VCC_PSINTFP alarm interrupt has occurred.The required processing should be put here.
			printf("\r\nAlarm X - PL_VCC_PSINTFP(SUPPLY5) alarm has occurred \r\n");
			break;
		}
		if (PL_VCC_PSAUXIntr == TRUE)
		{
			//Alarm X - PL_VCC_PSAUX alarm interrupt has occurred.The required processing should be put here.
			printf("\r\nAlarm X - PL_VCC_PSAUX(SUPPLY6) alarm has occurred \r\n");
			break;
		}

		if (PL_VUser0Intr == TRUE)
		{
			//Alarm 8 - PL_VUser0 alarm interrupt has occurred.The required processing should be put here.
			printf("\r\nAlarm 8 - PL_VUser0(SUPPLY7) alarm has occurred \r\n");
			break;
		}
		if (PL_VUser1Intr == TRUE)
		{
			//Alarm 9 - PL_VUser1 alarm interrupt has occurred.The required processing should be put here.
			printf("\r\nAlarm 9 - PL_VUser1(SUPPLY8) alarm has occurred \r\n");
			break;
		}
		if (PL_VUser2Intr == TRUE)
		{
			//Alarm 10 - PL_VUser2 alarm interrupt has occurred.The required processing should be put here.
			printf("\r\nAlarm 10 - PL_VUser2(SUPPLY9) alarm has occurred \r\n");
			break;
		}
		if (PL_VUser3Intr == TRUE)
		{
			//Alarm 11 - PL_VUser3 alarm interrupt has occurred.The required processing should be put here.
			printf("\r\nAlarm 11 - PL_VUser0(SUPPLY10) alarm has occurred \r\n");
			break;
		}

	}

	printf("\r\nExiting the PL_SysMon Interrupt Example. \r\n");

	return XST_SUCCESS;
}
/*****************************************************************************/
/**
*
* This function is the Interrupt Service Routine for the System Monitor device.
* It will be called by the processor whenever an interrupt is asserted
* by the device.
*
* This function only handles ALARM 0, ALARM 1 and ALARM 2 interrupts.
* User of this code may need to modify the code to meet the needs of the
* application.
*
* @param	CallBackRef is the callback reference passed from the Interrupt
*		controller driver, which in our case is a pointer to the
*		driver instance.
*
* @return	None.
*
* @note		This function is called within interrupt context.
*
******************************************************************************/
static void SysMonPsuInterruptHandler(void *CallBackRef)
{
	u64 IntrStatusValue;
	XSysMonPsu *SysMonPtr = (XSysMonPsu *)CallBackRef;

	IntrStatusValue = XSysMonPsu_IntrGetStatus(SysMonPtr);//Get the interrupt status from the device and check the value

	if (IntrStatusValue & XSYSMONPSU_ISR_0_PS_ALM_0_MASK)
	{
		TempLPIntrActive = TRUE;//Set LP Temperature interrupt flag so the code in application context can be aware of this interrupt.
		XSysMonPsu_IntrClear(SysMonPtr, IntrStatusValue);//Clear all bits in Interrupt Status Register(PS)
	}

	if (IntrStatusValue & XSYSMONPSU_ISR_0_PS_ALM_1_MASK)
	{
		VCC_PSINTLPIntr = TRUE;//Set VCC_PSINTLP(SUPPLY1) interrupt flag so the code in application context can be aware of this interrupt.
		XSysMonPsu_IntrClear(SysMonPtr, IntrStatusValue);//Clear all bits in Interrupt Status Register(PS)
	}

	if (IntrStatusValue & XSYSMONPSU_ISR_0_PS_ALM_2_MASK)
	{
		VCCINTFPIntr = TRUE;//Set VCCINTFP(SUPPLY2) interrupt flag so the code in application context can be aware of this interrupt.
		XSysMonPsu_IntrClear(SysMonPtr, IntrStatusValue);//Clear all bits in Interrupt Status Register(PS)
	}

	if (IntrStatusValue & XSYSMONPSU_ISR_0_PS_ALM_3_MASK)
	{
		VCC_PSAUXIntr = TRUE;//Set VCC_PSAUX(SUPPLY3) interrupt flag so the code in application context can be aware of this interrupt.
		XSysMonPsu_IntrClear(SysMonPtr, IntrStatusValue);//Clear all bits in Interrupt Status Register(PS)
	}

	if (IntrStatusValue & XSYSMONPSU_ISR_0_PS_ALM_4_MASK)
	{
		VCC_PSINTFP_DDR_504Intr = TRUE;//Set VCC_PSDDR_504(SUPPLY4) interrupt flag so the code in application context can be aware of this interrupt.
		XSysMonPsu_IntrClear(SysMonPtr, IntrStatusValue);//Clear all bits in Interrupt Status Register(PS)
	}

	/*if (IntrStatusValue & XSYSMONPSU_ISR_0_PS_ALM_5_MASK)
	{
		VCC_PSIO3_503Intr = TRUE;//Set VCC_PSIO3_503(SUPPLY5) interrupt flag so the code in application context can be aware of this interrupt.
		XSysMonPsu_IntrClear(SysMonPtr, IntrStatusValue);//Clear all bits in Interrupt Status Register(PS)
	}

	if (IntrStatusValue & XSYSMONPSU_ISR_0_PS_ALM_6_MASK)
	{
		VCC_PSIO0_500Intr = TRUE;//Set VCC_PSIO0_500(SUPPLY6) interrupt flag so the code in application context can be aware of this interrupt.
		XSysMonPsu_IntrClear(SysMonPtr, IntrStatusValue);//Clear all bits in Interrupt Status Register(PS)
	}

	if (IntrStatusValue & XSYSMONPSU_ISR_0_PS_ALM_8_MASK)
	{
		VCC_PSIO1_501Intr = TRUE;//Set VCC_PSIO1_501(SUPPLY7) interrupt flag so the code in application context can be aware of this interrupt.
		XSysMonPsu_IntrClear(SysMonPtr, IntrStatusValue);//Clear all bits in Interrupt Status Register(PS)
	}

	if (IntrStatusValue & XSYSMONPSU_ISR_0_PS_ALM_9_MASK)
	{
		VCC_PSIO2_502Intr = TRUE;//Set VCC_PSIO2_502(SUPPLY8) interrupt flag so the code in application context can be aware of this interrupt.
		XSysMonPsu_IntrClear(SysMonPtr, IntrStatusValue);//Clear all bits in Interrupt Status Register(PS)
	}*/

	if (IntrStatusValue & XSYSMONPSU_ISR_0_PS_ALM_10_MASK)
	{
		PS_MGTRAVCCIntr = TRUE;//Set PS_MGTRAVCC(SUPPLY9) interrupt flag so the code in application context can be aware of this interrupt.
		XSysMonPsu_IntrClear(SysMonPtr, IntrStatusValue);//Clear all bits in Interrupt Status Register(PS)
	}

	if (IntrStatusValue & XSYSMONPSU_ISR_0_PS_ALM_11_MASK)
	{
		PS_MGTRAVTTIntr = TRUE;//Set PS_MGTRAVTT(SUPPLY10) interrupt flag so the code in application context can be aware of this interrupt.
		XSysMonPsu_IntrClear(SysMonPtr, IntrStatusValue);//Clear all bits in Interrupt Status Register(PS)
	}

	if (IntrStatusValue & XSYSMONPSU_ISR_0_PS_ALM_12_MASK)
	{
		VCCAMSIntr = TRUE;//Set VCCAMS interrupt flag so the code in application context can be aware of this interrupt.
		XSysMonPsu_IntrClear(SysMonPtr, IntrStatusValue);//Clear all bits in Interrupt Status Register(PS)
	}

	if (IntrStatusValue & XSYSMONPSU_ISR_0_PS_ALM_13_MASK)
	{
		TempFPIntrActive = TRUE;//Set FP Temperature interrupt flag so the code in application context can be aware of this interrupt.
		XSysMonPsu_IntrClear(SysMonPtr, IntrStatusValue);//Clear all bits in Interrupt Status Register(PS)
	}



	if (IntrStatusValue & XSYSMONPSU_ISR_0_PL_ALM_0_MASK)
	{
		PL_TempActive = TRUE;//Set LP Temperature interrupt flag so the code in application context can be aware of this interrupt.
		XSysMonPsu_IntrClear(SysMonPtr, IntrStatusValue);//Clear all bits in Interrupt Status Register(PL)
	}

	if (IntrStatusValue & XSYSMONPSU_ISR_0_PL_ALM_1_MASK)
	{
		PL_VCCINTIntr = TRUE;//Set PL_VCCINT(SUPPLY1) interrupt flag so the code in application context can be aware of this interrupt.
		XSysMonPsu_IntrClear(SysMonPtr, IntrStatusValue);//Clear all bits in Interrupt Status Register(PL)
	}

	if (IntrStatusValue & XSYSMONPSU_ISR_0_PL_ALM_2_MASK)
	{
		PL_VCCAUXIntr = TRUE;//Set PL_VCCAUX(SUPPLY2) interrupt flag so the code in application context can be aware of this interrupt.
		XSysMonPsu_IntrClear(SysMonPtr, IntrStatusValue);//Clear all bits in Interrupt Status Register(PL)
	}

	if (IntrStatusValue & XSYSMONPSU_ISR_0_PL_ALM_3_MASK)
	{
		PL_VCCBRAMIntr = TRUE;//Set PL_VCCBRAM(SUPPLY3) interrupt flag so the code in application context can be aware of this interrupt.
		XSysMonPsu_IntrClear(SysMonPtr, IntrStatusValue);//Clear all bits in Interrupt Status Register(PL)
	}

	/*if (IntrStatusValue & XSYSMONPSU_ISR_0_PL_ALM_7_MASK)
	{
		PL_VREFPIntr = TRUE;//Set PL_VREFP interrupt flag so the code in application context can be aware of this interrupt.
		XSysMonPsu_IntrClear(SysMonPtr, IntrStatusValue);//Clear all bits in Interrupt Status Register(PL)
	}

	if (IntrStatusValue & XSYSMONPSU_ISR_0_PL_ALM_8_MASK)
	{
		PL_VREFNIntr = TRUE;//Set PL_VREFN interrupt flag so the code in application context can be aware of this interrupt.
		XSysMonPsu_IntrClear(SysMonPtr, IntrStatusValue);//Clear all bits in Interrupt Status Register(PL)
	}

	if (IntrStatusValue & XSYSMONPSU_ISR_0_PL_ALM_6_MASK)//XSYSMONPSU_ISR_0_PL_ALM_1_MASK
	{
		PL_VP_VNIntr = TRUE;//Set PL_VP_VN interrupt flag so the code in application context can be aware of this interrupt.
		XSysMonPsu_IntrClear(SysMonPtr, IntrStatusValue);//Clear all bits in Interrupt Status Register(PL)
	}*/

	if (IntrStatusValue & XSYSMONPSU_ISR_0_PL_ALM_4_MASK)
	{
		PL_VCC_PSINTLPIntr = TRUE;//Set PL_VCC_PSINTLP(SUPPLY4) interrupt flag so the code in application context can be aware of this interrupt.
		XSysMonPsu_IntrClear(SysMonPtr, IntrStatusValue);//Clear all bits in Interrupt Status Register(PL)
	}

	if (IntrStatusValue & XSYSMONPSU_ISR_0_PL_ALM_5_MASK)
	{
		PL_VCC_PSINTFPIntr = TRUE;//Set PL_VCC_PSINTFP(SUPPLY5) interrupt flag so the code in application context can be aware of this interrupt.
		XSysMonPsu_IntrClear(SysMonPtr, IntrStatusValue);//Clear all bits in Interrupt Status Register(PL)
	}

	if (IntrStatusValue & XSYSMONPSU_ISR_0_PL_ALM_6_MASK)
	{
		PL_VCC_PSAUXIntr = TRUE;//Set PL_VCC_PSAUX(SUPPLY6) interrupt flag so the code in application context can be aware of this interrupt.
		XSysMonPsu_IntrClear(SysMonPtr, IntrStatusValue);//Clear all bits in Interrupt Status Register(PL)
	}

	if (IntrStatusValue & XSYSMONPSU_ISR_0_PL_ALM_10_MASK)
	{
		PL_VUser0Intr = TRUE;//Set PL_VUser0(SUPPLY7) interrupt flag so the code in application context can be aware of this interrupt.
		XSysMonPsu_IntrClear(SysMonPtr, IntrStatusValue);//Clear all bits in Interrupt Status Register(PL)
	}

	if (IntrStatusValue & XSYSMONPSU_ISR_0_PL_ALM_11_MASK)
	{
		PL_VUser1Intr = TRUE;//Set PL_VUser1(SUPPLY8) interrupt flag so the code in application context can be aware of this interrupt.
		XSysMonPsu_IntrClear(SysMonPtr, IntrStatusValue);//Clear all bits in Interrupt Status Register(PL)
	}

	if (IntrStatusValue & XSYSMONPSU_ISR_0_PL_ALM_12_MASK)
	{
		PL_VUser2Intr = TRUE;//Set PL_VUser2(SUPPLY9) interrupt flag so the code in application context can be aware of this interrupt.
		XSysMonPsu_IntrClear(SysMonPtr, IntrStatusValue);//Clear all bits in Interrupt Status Register(PL)
	}

	if (IntrStatusValue & XSYSMONPSU_ISR_0_PL_ALM_13_MASK)
	{
		PL_VUser3Intr = TRUE;//Set PL_VUser3(SUPPLY10) interrupt flag so the code in application context can be aware of this interrupt.
		XSysMonPsu_IntrClear(SysMonPtr, IntrStatusValue);//Clear all bits in Interrupt Status Register(PL)
	}



	//Disable Alarm 0 interrupt for on-chip temperature,Alarm 1 interrupt for on-chip VCCINT and Alarm 3 interrupt for on-chip VCCAUX.
	XSysMonPsu_IntrDisable(SysMonPtr, XSYSMONPSU_IER_0_PS_ALM_0_MASK | XSYSMONPSU_IER_0_PS_ALM_1_MASK | XSYSMONPSU_IER_0_PS_ALM_2_MASK | XSYSMONPSU_IER_0_PS_ALM_3_MASK |
			XSYSMONPSU_IER_0_PS_ALM_4_MASK | XSYSMONPSU_IER_0_PS_ALM_5_MASK | XSYSMONPSU_IER_0_PS_ALM_6_MASK | XSYSMONPSU_IER_0_PS_ALM_7_MASK | XSYSMONPSU_IER_0_PS_ALM_8_MASK |
			XSYSMONPSU_IER_0_PS_ALM_9_MASK | XSYSMONPSU_IER_0_PS_ALM_10_MASK | XSYSMONPSU_IER_0_PS_ALM_11_MASK | XSYSMONPSU_IER_0_PS_ALM_12_MASK | XSYSMONPSU_IER_0_PS_ALM_13_MASK |
			XSYSMONPSU_IER_0_PS_ALM_14_MASK |XSYSMONPSU_IER_0_PS_ALM_15_MASK );
	XSysMonPsu_IntrDisable(SysMonPtr, XSYSMONPSU_IER_0_PL_ALM_0_MASK | XSYSMONPSU_IER_0_PL_ALM_1_MASK | XSYSMONPSU_IER_0_PL_ALM_2_MASK | XSYSMONPSU_IER_0_PL_ALM_3_MASK |
			XSYSMONPSU_IER_0_PL_ALM_4_MASK | XSYSMONPSU_IER_0_PL_ALM_5_MASK | XSYSMONPSU_IER_0_PL_ALM_6_MASK | XSYSMONPSU_IER_0_PL_ALM_7_MASK | XSYSMONPSU_IER_0_PL_ALM_8_MASK |
			XSYSMONPSU_IER_0_PL_ALM_9_MASK | XSYSMONPSU_IER_0_PL_ALM_10_MASK | XSYSMONPSU_IER_0_PL_ALM_11_MASK | XSYSMONPSU_IER_0_PL_ALM_12_MASK | XSYSMONPSU_IER_0_PL_ALM_13_MASK |
			XSYSMONPSU_IER_0_PL_ALM_14_MASK |XSYSMONPSU_IER_0_PL_ALM_15_MASK );
 }

/****************************************************************************/
/**
*
* This function sets up the interrupt system so interrupts can occur for the
* System Monitor.  The function is application-specific since the actual
* system may or may not have an interrupt controller. The System Monitor
* device could be directly connected to a processor without an interrupt
* controller. The user should modify this function to fit the application.
*
* @param	XScuGicInstPtr is a pointer to the Interrupt Controller driver
*		Instance.
* @param	SysMonPtr is a pointer to the driver instance for the System
* 		Monitor device which is going to be connected to the interrupt
*		controller.
* @param	IntrId is XPAR_<SYSMON_instance>_INTR
*		value from xparameters_ps.h
*
* @return	XST_SUCCESS if successful, or XST_FAILURE.
*
* @note		None.
*
*
****************************************************************************/
static int SysMonPsuSetupInterruptSystem(XScuGic* XScuGicInstPtr, XSysMonPsu *SysMonPtr, u16 IntrId )//INTR_ID = XPAR_XSYSMONPSU_INTR = XPS_AMS_INT_ID = (56U + 32U)
{
	int Status;

#ifndef TESTAPP_GEN
	XScuGic_Config *XScuGicConfig; /* Configure for interrupt controller */

	XScuGicConfig = XScuGic_LookupConfig(XSCUGIC_DEVICE_ID);//Initialize the interrupt controller driver
	if (NULL == XScuGicConfig)
	{
		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(XScuGicInstPtr, XScuGicConfig, XScuGicConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}

	// Connect the interrupt controller interrupt handler to the hardware interrupt handling logic in the processor.
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler) XScuGic_InterruptHandler, XScuGicInstPtr);
#endif

	 // Connect a device driver handler that will be called when an interrupt for the device occurs, the device driver handler performs the specific interrupt processing for the device
	Status = XScuGic_Connect(XScuGicInstPtr, IntrId, (Xil_ExceptionHandler) SysMonPsuInterruptHandler, (void *) SysMonPtr);
	if (Status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}

	XScuGic_Enable(XScuGicInstPtr, IntrId);//Enable the interrupt for the device

#ifndef TESTAPP_GEN
	 Xil_ExceptionEnable();//Enable exception interrupts
#endif
	return XST_SUCCESS;
}

/****************************************************************************/
/**
*
* This function converts the fraction part of the given floating point number
* (after the decimal point)to an integer.
*
* @param	FloatNum is the floating point number.
*
* @return	Integer number to a precision of 3 digits.
*
* @note
* This function is used in the printing of floating point data to a STDIO
* device using the xil_printf function. The xil_printf is a very small
* foot-print printf function and does not support the printing of floating
* point numbers.
*
*****************************************************************************/
int SysMonPsuFractionToInt(float FloatNum)
{
	float Temp;
	Temp = FloatNum;
	if (FloatNum < 0)
	{
		Temp = -(FloatNum);
	}
	return( ((int)((Temp -(float)((int)Temp)) * (1000.0f))));
}

