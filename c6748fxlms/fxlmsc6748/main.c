/**
 *
 * author : junyeong Heo
 * e-mail : jyheo@yonsei.ac.kr
 *
 * \file  fxlms.c
 *
 * \brief Sample application for fxlms.
 *        An application program that controls
 *        input noise from LINE_IN of EVM to LINE_OUT of EVM.
 *        using narrowband FxLMS algorithm.
 */

#include "edma_event.h" 
#include "interrupt.h"
#include "soc_C6748.h"
#include "hw_syscfg0_C6748.h"
#include "lcdkC6748.h"
#include "codecif.h"
#include "mcasp.h"
#include "aic31.h"
#include "edma.h"
#include "psc.h"

#include <string.h>

#include "common.h"

/******************************************************************************
**                      INTERNAL MACRO DEFINITIONS
******************************************************************************/
/*
** Values which are configurable
*/
/* Slot size to send/receive data */
#define SLOT_SIZE                             (16u)

/* Word size to send/receive data. Word size <= Slot size */
#define WORD_SIZE                             (16u)

/* Sampling Rate which will be used by both transmit and receive sections */
#define SAMPLING_RATE                         (4000u)

/* Number of channels, L & R */
#define NUM_I2S_CHANNELS                      (2u)

/* Number of samples to be used per audio buffer */
#define NUM_SAMPLES_PER_AUDIO_BUF             (10u)

/* Number of buffers used per tx/rx */
#define NUM_BUF                               (3u)

/* Number of linked parameter set used per tx/rx */
#define NUM_PAR                               (2u)

/* Specify where the parameter set starting is */
#define PAR_ID_START                          (40u)

/* Number of samples in loop buffer */
#define NUM_SAMPLES_LOOP_BUF                  (10u)

/* AIC3106 codec address */
#define I2C_SLAVE_CODEC_AIC31                 (0x18u) 

/* Interrupt channels to map in AINTC */
#define INT_CHANNEL_I2C                       (2u)
#define INT_CHANNEL_MCASP                     (2u)
#define INT_CHANNEL_EDMACC                    (2u)

/* McASP Serializer for Receive */
#define MCASP_XSER_RX                         (14u)

/* McASP Serializer for Transmit */
#define MCASP_XSER_TX                         (13u)

/*
** Below Macros are calculated based on the above inputs
*/
#define NUM_TX_SERIALIZERS                    ((NUM_I2S_CHANNELS >> 1) \
                                               + (NUM_I2S_CHANNELS & 0x01))
#define NUM_RX_SERIALIZERS                    ((NUM_I2S_CHANNELS >> 1) \
                                               + (NUM_I2S_CHANNELS & 0x01))
#define I2S_SLOTS                             ((1 << NUM_I2S_CHANNELS) - 1)

#define BYTES_PER_SAMPLE                      ((WORD_SIZE >> 3) \
                                               * NUM_I2S_CHANNELS)

#define AUDIO_BUF_SIZE                        (NUM_SAMPLES_PER_AUDIO_BUF \
                                               * BYTES_PER_SAMPLE)

#define TX_DMA_INT_ENABLE                     (EDMA3CC_OPT_TCC_SET(1) | (1 \
                                               << EDMA3CC_OPT_TCINTEN_SHIFT))
#define RX_DMA_INT_ENABLE                     (EDMA3CC_OPT_TCC_SET(0) | (1 \
                                               << EDMA3CC_OPT_TCINTEN_SHIFT))

#define PAR_RX_START                          (PAR_ID_START)
#define PAR_TX_START                          (PAR_RX_START + NUM_PAR)

/*
** Definitions which are not configurable 
*/
#define SIZE_PARAMSET                         (32u)
#define OPT_FIFO_WIDTH                        (0x02 << 8u)

/******************************************************************************
**                      MACRO FUNCTION PROTOTYPES
******************************************************************************/

#define MEM_TRANSFER(in, out, size) memcpy((void *)out, (void *)in, size);
#define REFERENCE_SINUSOID_GENERATOR(a1, b0, qn0, qn1, qn2, cos, sin) \
        qn0 = 2 * a1 * qn1 - qn2;\
        qn2 = qn1;\
        qn1 = qn0;\
        cos = qn1 - a1 * qn2;\

        sin = b0 * qn2;

#define FIR_FILTERING(x, y, out, size) \
        for(i = 0; i < size; i++) out += x[i]*y[i];

/******************************************************************************
**                      INTERNAL FUNCTION PROTOTYPES
******************************************************************************/
static void McASPErrorIsr(void);
static void McASPErrorIntSetup(void);
static void AIC31I2SConfigure(void);
static void McASPI2SConfigure(void);
static void McASPTxDMAComplHandler(void);
static void McASPRxDMAComplHandler(void);
static void EDMA3CCComplIsr(void);
static void I2SDataTxRxActivate(void);
static void I2SDMAParamInit(void);
static void ParamTxLoopJobSet(unsigned short parId);
static void BufferTxDMAActivate(unsigned int txBuf, unsigned short numSamples,
                                unsigned short parToUpdate, 
                                unsigned short linkAddr);
static void BufferRxDMAActivate(unsigned int rxBuf, unsigned short parId,
                                unsigned short parLink);

/******************************************************************************
**                      INTERNAL VARIABLE DEFINITIONS
******************************************************************************/

static int i, j, k; // for iterations


static unsigned char loopBuf[NUM_SAMPLES_LOOP_BUF * BYTES_PER_SAMPLE] = {0};

/*
** Transmit buffers. If any new buffer is to be added, define it here and 
** update the NUM_BUF.
*/
static unsigned char txBuf0[AUDIO_BUF_SIZE];
static unsigned char txBuf1[AUDIO_BUF_SIZE];
static unsigned char txBuf2[AUDIO_BUF_SIZE];

/*
** Receive buffers. If any new buffer is to be added, define it here and 
** update the NUM_BUF.
*/
static unsigned char rxBuf0[AUDIO_BUF_SIZE];
static unsigned char rxBuf1[AUDIO_BUF_SIZE];
static unsigned char rxBuf2[AUDIO_BUF_SIZE];

/*
** Processing buffers. If any new buffer is to be added, define it here and
** update the NUM_BUF.
*/

static unsigned char pcBuf0[AUDIO_BUF_SIZE];
static unsigned char pcBuf1[AUDIO_BUF_SIZE];
static unsigned char pcBuf2[AUDIO_BUF_SIZE];

static float pcfBuf0[AUDIO_BUF_SIZE];
static float pcfBuf1[AUDIO_BUF_SIZE];
static float pcfBuf2[AUDIO_BUF_SIZE];



/*
** Next buffer to receive data. The data will be received in this buffer.
*/
static volatile unsigned int nxtBufToRcv = 0;

/*
** The RX buffer which filled latest.
*/
static volatile unsigned int lastFullRxBuf = 0;

/*
** The offset of the paRAM ID, from the starting of the paRAM set.
*/
static volatile unsigned short parOffRcvd = 0;

/*
** The offset of the paRAM ID sent, from starting of the paRAM set.
*/
static volatile unsigned short parOffSent = 0;

/*
** The offset of the paRAM ID to be sent next, from starting of the paRAM set. 
*/
static volatile unsigned short parOffTxToSend = 0;

/*
** The transmit buffer which was sent last.
*/
static volatile unsigned int lastSentTxBuf = NUM_BUF - 1;

/******************************************************************************
**                      INTERNAL CONSTATNT DEFINITIONS
******************************************************************************/
/* Array of receive buffer pointers */
static unsigned int const rxBufPtr[NUM_BUF] =
       { 
           (unsigned int) rxBuf0,
           (unsigned int) rxBuf1,
           (unsigned int) rxBuf2
       };

/* Array of transmit buffer pointers */
static unsigned int const txBufPtr[NUM_BUF] =
       { 
           (unsigned int) txBuf0,
           (unsigned int) txBuf1,
           (unsigned int) txBuf2
       };


/* Array of processing buffer pointers */
static unsigned int const pcfBufPtr[NUM_BUF] =
       {
           (unsigned int) pcfBuf0,
           (unsigned int) pcfBuf1,
           (unsigned int) pcfBuf2
       };

/*
** Default paRAM for Transmit section. This will be transmitting from 
** a loop buffer.
*/
static struct EDMA3CCPaRAMEntry const txDefaultPar = 
       {
           (unsigned int)(EDMA3CC_OPT_DAM  | (0x02 << 8u)), /* Opt field */
           (unsigned int)loopBuf, /* source address */
           (unsigned short)(BYTES_PER_SAMPLE), /* aCnt */
           (unsigned short)(NUM_SAMPLES_LOOP_BUF), /* bCnt */ 
           (unsigned int) SOC_MCASP_0_DATA_REGS, /* dest address */
           (short) (BYTES_PER_SAMPLE), /* source bIdx */
           (short)(0), /* dest bIdx */
           (unsigned short)(PAR_TX_START * SIZE_PARAMSET), /* link address */
           (unsigned short)(0), /* bCnt reload value */
           (short)(0), /* source cIdx */
           (short)(0), /* dest cIdx */
           (unsigned short)1 /* cCnt */
       };

/*
** Default paRAM for Receive section.  
*/
static struct EDMA3CCPaRAMEntry const rxDefaultPar =
       {
           (unsigned int)(EDMA3CC_OPT_SAM  | (0x02 << 8u)), /* Opt field */
           (unsigned int)SOC_MCASP_0_DATA_REGS, /* source address */
           (unsigned short)(BYTES_PER_SAMPLE), /* aCnt */
           (unsigned short)(1), /* bCnt */
           (unsigned int)rxBuf0, /* dest address */
           (short) (0), /* source bIdx */
           (short)(BYTES_PER_SAMPLE), /* dest bIdx */
           (unsigned short)(PAR_RX_START * SIZE_PARAMSET), /* link address */
           (unsigned short)(0), /* bCnt reload value */
           (short)(0), /* source cIdx */
           (short)(0), /* dest cIdx */
           (unsigned short)1 /* cCnt */
       };

static struct ReferenceSignal refSignal = {0, 1, 0, 0, 0, 0, 0, 0, 0};

/******************************************************************************
**                          FUNCTION DEFINITIONS
******************************************************************************/
/*
** Assigns loop job for a parameter set
*/
static void ParamTxLoopJobSet(unsigned short parId)
{
    EDMA3CCPaRAMEntry paramSet;
    
    memcpy(&paramSet, &txDefaultPar, SIZE_PARAMSET - 2);
  
    /* link the paRAM to itself */
    paramSet.linkAddr = parId * SIZE_PARAMSET;

    EDMA3SetPaRAM(SOC_EDMA30CC_0_REGS, parId, &paramSet);
}

/*
** Initializes the DMA parameters.
** The RX basic paRAM set(channel) is 0 and TX basic paRAM set (channel) is 1.
**
** The RX paRAM set 0 will be initialized to receive data in the rx buffer 0.
** The transfer completion interrupt will not be enabled for paRAM set 0;
** paRAM set 0 will be linked to linked paRAM set starting (PAR_RX_START) of RX.
** and further reception only happens via linked paRAM set. 
** For example, if the PAR_RX_START value is 40, and the number of paRAMS is 2, 
** reception paRAM set linking will be initialized as 0-->40-->41-->40
**
** The TX paRAM sets will be initialized to transmit from the loop buffer.
** The size of the loop buffer can be configured.   
** The transfer completion interrupt will not be enabled for paRAM set 1;
** paRAM set 1 will be linked to linked paRAM set starting (PAR_TX_START) of TX.
** All other paRAM sets will be linked to itself.
** and further transmission only happens via linked paRAM set.
** For example, if the PAR_RX_START value is 42, and the number of paRAMS is 2, 
** So transmission paRAM set linking will be initialized as 1-->42-->42, 43->43. 
*/
static void I2SDMAParamInit(void)
{
    EDMA3CCPaRAMEntry paramSet;
    int idx; 
 
    /* Initialize the 0th paRAM set for receive */ 
    memcpy(&paramSet, &rxDefaultPar, SIZE_PARAMSET - 2);

    EDMA3SetPaRAM(SOC_EDMA30CC_0_REGS, EDMA3_CHA_MCASP0_RX, &paramSet);

    /* further paramsets, enable interrupt */
    paramSet.opt |= RX_DMA_INT_ENABLE; 
 
    for(idx = 0 ; idx < NUM_PAR; idx++)
    {
        paramSet.destAddr = rxBufPtr[idx];

        paramSet.linkAddr = (PAR_RX_START + ((idx + 1) % NUM_PAR)) 
                             * (SIZE_PARAMSET);        

        paramSet.bCnt =  NUM_SAMPLES_PER_AUDIO_BUF;

        /* 
        ** for the first linked paRAM set, start receiving the second
        ** sample only since the first sample is already received in
        ** rx buffer 0 itself.
        */
        if( 0 == idx)
        {
            paramSet.destAddr += BYTES_PER_SAMPLE;
            paramSet.bCnt -= BYTES_PER_SAMPLE;
        }

        EDMA3SetPaRAM(SOC_EDMA30CC_0_REGS, (PAR_RX_START + idx), &paramSet);
    } 

    /* Initialize the required variables for reception */
    nxtBufToRcv = idx % NUM_BUF;
    lastFullRxBuf = NUM_BUF - 1;
    parOffRcvd = 0;

    /* Initialize the 1st paRAM set for transmit */ 
    memcpy(&paramSet, &txDefaultPar, SIZE_PARAMSET);

    EDMA3SetPaRAM(SOC_EDMA30CC_0_REGS, EDMA3_CHA_MCASP0_TX, &paramSet);

    /* rest of the params, enable loop job */
    for(idx = 0 ; idx < NUM_PAR; idx++)
    {
        ParamTxLoopJobSet(PAR_TX_START + idx);
    }
 
    /* Initialize the variables for transmit */
    parOffSent = 0;
    lastSentTxBuf = NUM_BUF - 1; 
}

/*
** Function to configure the codec for I2S mode
*/
static void AIC31I2SConfigure(void)
{
    volatile unsigned int delay = 0xFFF;

    AIC31Reset(SOC_I2C_0_REGS);
    while(delay--);

    /* Configure the data format and sampling rate */
    AIC31DataConfig(SOC_I2C_0_REGS, AIC31_DATATYPE_I2S, SLOT_SIZE, 0);
    AIC31SampleRateConfig(SOC_I2C_0_REGS, AIC31_MODE_BOTH, SAMPLING_RATE);

    /* Initialize both ADC and DAC */
    AIC31ADCInit(SOC_I2C_0_REGS);
    AIC31DACInit(SOC_I2C_0_REGS);
}

/*
** Configures the McASP Transmit Section in I2S mode.
*/
static void McASPI2SConfigure(void)
{
    McASPRxReset(SOC_MCASP_0_CTRL_REGS);
    McASPTxReset(SOC_MCASP_0_CTRL_REGS);

    /* Enable the FIFOs for DMA transfer */
    McASPReadFifoEnable(SOC_MCASP_0_FIFO_REGS, 1, 1);
    McASPWriteFifoEnable(SOC_MCASP_0_FIFO_REGS, 1, 1);

    /* Set I2S format in the transmitter/receiver format units */
    McASPRxFmtI2SSet(SOC_MCASP_0_CTRL_REGS, WORD_SIZE, SLOT_SIZE,
                     MCASP_RX_MODE_DMA);
    McASPTxFmtI2SSet(SOC_MCASP_0_CTRL_REGS, WORD_SIZE, SLOT_SIZE,
                     MCASP_TX_MODE_DMA);

    /* Configure the frame sync. I2S shall work in TDM format with 2 slots */
    McASPRxFrameSyncCfg(SOC_MCASP_0_CTRL_REGS, 2, MCASP_RX_FS_WIDTH_WORD, 
                        MCASP_RX_FS_EXT_BEGIN_ON_FALL_EDGE);
    McASPTxFrameSyncCfg(SOC_MCASP_0_CTRL_REGS, 2, MCASP_TX_FS_WIDTH_WORD, 
                        MCASP_TX_FS_EXT_BEGIN_ON_RIS_EDGE);

    /* configure the clock for receiver */
    McASPRxClkCfg(SOC_MCASP_0_CTRL_REGS, MCASP_RX_CLK_EXTERNAL, 0, 0);
    McASPRxClkPolaritySet(SOC_MCASP_0_CTRL_REGS, MCASP_RX_CLK_POL_RIS_EDGE); 
    McASPRxClkCheckConfig(SOC_MCASP_0_CTRL_REGS, MCASP_RX_CLKCHCK_DIV32,
                          0x00, 0xFF);

    /* configure the clock for transmitter */
    McASPTxClkCfg(SOC_MCASP_0_CTRL_REGS, MCASP_TX_CLK_EXTERNAL, 0, 0);
    McASPTxClkPolaritySet(SOC_MCASP_0_CTRL_REGS, MCASP_TX_CLK_POL_FALL_EDGE); 
    McASPTxClkCheckConfig(SOC_MCASP_0_CTRL_REGS, MCASP_TX_CLKCHCK_DIV32,
                          0x00, 0xFF);
 
    /* Enable synchronization of RX and TX sections  */  
    McASPTxRxClkSyncEnable(SOC_MCASP_0_CTRL_REGS);

    /* Enable the transmitter/receiver slots. I2S uses 2 slots */
    McASPRxTimeSlotSet(SOC_MCASP_0_CTRL_REGS, I2S_SLOTS);
    McASPTxTimeSlotSet(SOC_MCASP_0_CTRL_REGS, I2S_SLOTS);

    /*
    ** Set the serializers, Currently only one serializer is set as
    ** transmitter and one serializer as receiver.
    */
    McASPSerializerRxSet(SOC_MCASP_0_CTRL_REGS, MCASP_XSER_RX);
    McASPSerializerTxSet(SOC_MCASP_0_CTRL_REGS, MCASP_XSER_TX);

    /*
    ** Configure the McASP pins 
    ** Input - Frame Sync, Clock and Serializer Rx
    ** Output - Serializer Tx is connected to the input of the codec 
    */
    McASPPinMcASPSet(SOC_MCASP_0_CTRL_REGS, 0xFFFFFFFF);
    McASPPinDirOutputSet(SOC_MCASP_0_CTRL_REGS, MCASP_PIN_AXR(MCASP_XSER_TX));
    McASPPinDirInputSet(SOC_MCASP_0_CTRL_REGS, MCASP_PIN_AFSX 
                                               | MCASP_PIN_ACLKX
                                               | MCASP_PIN_AFSR
                                               | MCASP_PIN_ACLKR
                                               | MCASP_PIN_AXR(MCASP_XSER_RX));

    /* Enable error interrupts for McASP */
    McASPTxIntEnable(SOC_MCASP_0_CTRL_REGS, MCASP_TX_DMAERROR 
                                            | MCASP_TX_CLKFAIL 
                                            | MCASP_TX_SYNCERROR
                                            | MCASP_TX_UNDERRUN);

    McASPRxIntEnable(SOC_MCASP_0_CTRL_REGS, MCASP_RX_DMAERROR 
                                            | MCASP_RX_CLKFAIL
                                            | MCASP_RX_SYNCERROR 
                                            | MCASP_RX_OVERRUN);
}

/*
** Sets up the interrupts for EDMA in AINTC
*/
static void EDMA3IntSetup(void)
{
#ifdef _TMS320C6X
    IntRegister(C674X_MASK_INT5, EDMA3CCComplIsr);
    IntEventMap(C674X_MASK_INT5, SYS_INT_EDMA3_0_CC0_INT1);
    IntEnable(C674X_MASK_INT5);
#else
    IntRegister(SYS_INT_CCINT0, EDMA3CCComplIsr);
    IntChannelSet(SYS_INT_CCINT0, INT_CHANNEL_EDMACC); 
    IntSystemEnable(SYS_INT_CCINT0);
#endif
}

/*
** Sets up the error interrupts for McASP in AINTC
*/
static void McASPErrorIntSetup(void)
{
#ifdef _TMS320C6X
    IntRegister(C674X_MASK_INT6, McASPErrorIsr);
    IntEventMap(C674X_MASK_INT6, SYS_INT_MCASP0_INT);
    IntEnable(C674X_MASK_INT6);
#else
    /* Register the error ISR for McASP */
    IntRegister(SYS_INT_MCASPINT, McASPErrorIsr);

    IntChannelSet(SYS_INT_MCASPINT, INT_CHANNEL_MCASP);
    IntSystemEnable(SYS_INT_MCASPINT);
#endif
}

/*
** Activates the data transmission/reception
** The DMA parameters shall be ready before calling this function.
*/
static void I2SDataTxRxActivate(void)
{
    /* Start the clocks */
    McASPRxClkStart(SOC_MCASP_0_CTRL_REGS, MCASP_RX_CLK_EXTERNAL);
    McASPTxClkStart(SOC_MCASP_0_CTRL_REGS, MCASP_TX_CLK_EXTERNAL);

    /* Enable EDMA for the transfer */
    EDMA3EnableTransfer(SOC_EDMA30CC_0_REGS, EDMA3_CHA_MCASP0_RX,
                        EDMA3_TRIG_MODE_EVENT);
    EDMA3EnableTransfer(SOC_EDMA30CC_0_REGS, 
                        EDMA3_CHA_MCASP0_TX, EDMA3_TRIG_MODE_EVENT);

    /* Activate the  serializers */
    McASPRxSerActivate(SOC_MCASP_0_CTRL_REGS);
    McASPTxSerActivate(SOC_MCASP_0_CTRL_REGS);

    /* make sure that the XDATA bit is cleared to zero */
    while(McASPTxStatusGet(SOC_MCASP_0_CTRL_REGS) & MCASP_TX_STAT_DATAREADY);

    /* Activate the state machines */
    McASPRxEnable(SOC_MCASP_0_CTRL_REGS);
    McASPTxEnable(SOC_MCASP_0_CTRL_REGS);
}

/*
** Activates the DMA transfer for a parameterset from the given buffer.
*/
void BufferTxDMAActivate(unsigned int txBuf, unsigned short numSamples,
                         unsigned short parId, unsigned short linkPar)
{
    EDMA3CCPaRAMEntry paramSet;

    /* Copy the default paramset */
    memcpy(&paramSet, &txDefaultPar, SIZE_PARAMSET - 2);
    
    /* Enable completion interrupt */
    paramSet.opt |= TX_DMA_INT_ENABLE;
    paramSet.srcAddr =  txBufPtr[txBuf];
    paramSet.linkAddr = linkPar * SIZE_PARAMSET;  
    paramSet.bCnt = numSamples;

    EDMA3SetPaRAM(SOC_EDMA30CC_0_REGS, parId, &paramSet);
}


/*
** Activates the DMA transfer for a parameter set from the given buffer.
*/
static void BufferRxDMAActivate(unsigned int rxBuf, unsigned short parId,
                                unsigned short parLink)
{
    EDMA3CCPaRAMEntry paramSet;

    /* Copy the default paramset */
    memcpy(&paramSet, &rxDefaultPar, SIZE_PARAMSET - 2);

    /* Enable completion interrupt */
    paramSet.opt |= RX_DMA_INT_ENABLE;
    paramSet.destAddr =  rxBufPtr[rxBuf];
    paramSet.bCnt =  NUM_SAMPLES_PER_AUDIO_BUF;
    paramSet.linkAddr = parLink * SIZE_PARAMSET ;

    EDMA3SetPaRAM(SOC_EDMA30CC_0_REGS, parId, &paramSet);
}

/*
** This function will be called once receive DMA is completed
*/
static void McASPRxDMAComplHandler(void)
{
    unsigned short nxtParToUpdate;

    /*
    ** Update lastFullRxBuf to indicate a new buffer reception
    ** is completed.
    */
    lastFullRxBuf = (lastFullRxBuf + 1) % NUM_BUF;
    nxtParToUpdate =  PAR_RX_START + parOffRcvd;  
    parOffRcvd = (parOffRcvd + 1) % NUM_PAR;
 
    /*
    ** Update the DMA parameters for the received buffer to receive
    ** further data in proper buffer
    */
    BufferRxDMAActivate(nxtBufToRcv, nxtParToUpdate,
                        PAR_RX_START + parOffRcvd);
    
    /* update the next buffer to receive data */ 
    nxtBufToRcv = (nxtBufToRcv + 1) % NUM_BUF;
}

/*
** This function will be called once transmit DMA is completed
*/
static void McASPTxDMAComplHandler(void)
{
    ParamTxLoopJobSet((unsigned short)(PAR_TX_START + parOffSent));

    parOffSent = (parOffSent + 1) % NUM_PAR;
}

/*
** EDMA transfer completion ISR
*/
static void EDMA3CCComplIsr(void) 
{ 
#ifdef _TMS320C6X
	IntEventClear(SYS_INT_EDMA3_0_CC0_INT1);
#else
    IntSystemStatusClear(SYS_INT_CCINT0);
#endif

    /* Check if receive DMA completed */
    if(EDMA3GetIntrStatus(SOC_EDMA30CC_0_REGS) & (1 << EDMA3_CHA_MCASP0_RX)) 
    { 
        /* Clear the interrupt status for the 0th channel */
        EDMA3ClrIntr(SOC_EDMA30CC_0_REGS, EDMA3_CHA_MCASP0_RX); 
        McASPRxDMAComplHandler();
    }
    
    /* Check if transmit DMA completed */
    if(EDMA3GetIntrStatus(SOC_EDMA30CC_0_REGS) & (1 << EDMA3_CHA_MCASP0_TX)) 
    { 
        /* Clear the interrupt status for the first channel */
        EDMA3ClrIntr(SOC_EDMA30CC_0_REGS, EDMA3_CHA_MCASP0_TX); 
        McASPTxDMAComplHandler();
    }
}

/*
** Error ISR for McASP
*/
static void McASPErrorIsr(void)
{
#ifdef _TMS320C6X
	IntEventClear(SYS_INT_MCASP0_INT);
#else
    IntSystemStatusClear(SYS_INT_MCASPINT);
#endif

    ; /* Perform any error handling here.*/
}

static void FxLMSInit(void)
{
    /* Set up pin mux for I2C module 0 */
    I2CPinMuxSetup(0);
    McASPPinMuxSetup();

    /* Power up the McASP module */
    PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_MCASP0, PSC_POWERDOMAIN_ALWAYS_ON,
             PSC_MDCTL_NEXT_ENABLE);

    /* Power up EDMA3CC_0 and EDMA3TC_0 */
    PSCModuleControl(SOC_PSC_0_REGS, HW_PSC_CC0, PSC_POWERDOMAIN_ALWAYS_ON,
             PSC_MDCTL_NEXT_ENABLE);
    PSCModuleControl(SOC_PSC_0_REGS, HW_PSC_TC0, PSC_POWERDOMAIN_ALWAYS_ON,
             PSC_MDCTL_NEXT_ENABLE);

#ifdef _TMS320C6X
    // Initialize the DSP interrupt controller
    IntDSPINTCInit();
#else
    /* Initialize the ARM Interrupt Controller.*/
    IntAINTCInit();
#endif

    /* Initialize the I2C 0 interface for the codec AIC31 */
    I2CCodecIfInit(SOC_I2C_0_REGS, INT_CHANNEL_I2C, I2C_SLAVE_CODEC_AIC31);

    EDMA3Init(SOC_EDMA30CC_0_REGS, 0);
    EDMA3IntSetup();

    McASPErrorIntSetup();

#ifdef _TMS320C6X
    IntGlobalEnable();
#else
    /* Enable the interrupts generation at global level */
    IntMasterIRQEnable();
    IntGlobalEnable();
    IntIRQEnable();
#endif

    /*
    ** Request EDMA channels. Channel 0 is used for reception and
    ** Channel 1 is used for transmission
    */
    EDMA3RequestChannel(SOC_EDMA30CC_0_REGS, EDMA3_CHANNEL_TYPE_DMA,
                        EDMA3_CHA_MCASP0_TX, EDMA3_CHA_MCASP0_TX, 0);
    EDMA3RequestChannel(SOC_EDMA30CC_0_REGS, EDMA3_CHANNEL_TYPE_DMA,
                        EDMA3_CHA_MCASP0_RX, EDMA3_CHA_MCASP0_RX, 0);

    /* Initialize the DMA parameters */
    I2SDMAParamInit();

    /* Configure the Codec for I2S mode */
    AIC31I2SConfigure();

    /* Configure the McASP for I2S */
    McASPI2SConfigure();

    /* Activate the audio transmission and reception */
    I2SDataTxRxActivate();

}

/******************************************************************************
**                               MAIN FUNCTIONS
******************************************************************************/
/*
** The main function. Application starts here.
*/


int main(void)
{
    unsigned short parToSend;
    unsigned short parToLink;

    FxLMSInit();

    /*
    ** Looop forever. if a new buffer is received, the lastFullRxBuf will be
    ** updated in the rx completion ISR. if it is not the lastSentTxBuf,
    ** buffer is to be sent. This has to be mapped to proper paRAM set.
    */
    while(1)
    {

        if(lastFullRxBuf != lastSentTxBuf)
        {
            /*
            ** Start the transmission from the link paramset. The param set
            ** 1 will be linked to param set at PAR_TX_START. So do not
            ** update paRAM set1.
            */
            parToSend =  PAR_TX_START + (parOffTxToSend % NUM_PAR);
            parOffTxToSend = (parOffTxToSend + 1) % NUM_PAR;
            parToLink  = PAR_TX_START + parOffTxToSend;

            lastSentTxBuf = (lastSentTxBuf + 1) % NUM_BUF;

            /* Copy the buffer */
            MEM_TRANSFER(rxBufPtr[lastFullRxBuf], txBufPtr[lastSentTxBuf], AUDIO_BUF_SIZE);

            /*
            ** Send the buffer by setting the DMA params accordingly.
            ** Here the buffer to send and number of samples are passed as
            ** parameters. This is important, if only transmit section
            ** is to be used.
            */
            BufferTxDMAActivate(lastSentTxBuf, NUM_SAMPLES_PER_AUDIO_BUF,
                                (unsigned short)parToSend,
                                (unsigned short)parToLink);

            REFERENCE_SINUSOID_GENERATOR(cos(PI2*100/4000), sin(PI2*100/4000), \
                                         refSignal.qn0, refSignal.qn1, refSignal.qn2, \
                                         refSignal.cos_f0, refSignal.sin_f0);

            MEM_TRANSFER(rxBufPtr[lastFullRxBuf], pcBufPtr[lastSentTxtBuf], AUDIO_BUF_SIZE);

            /* fxlms */
            SHORT_TO_FLOAT_ARRAY()

        }
    }
}
/***************************** End Of File ***********************************/
