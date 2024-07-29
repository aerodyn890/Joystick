#include <zephyr/kernel.h>
#define JOYSTICK_SWITCH         6
#define JOYSTICK_AXIS_X         5
#define JOYSTICK_AXIS_Y         4

// Joystick Maximum
int16_t maximum[2] = {0};
int16_t minimum[2] = {0};

// SAADC Buffer
int16_t saadc_buffer[2] = {0};

// GPIOTE IRQ Handler
void GPIOTE_IRQHandler();
void TIMER1_IRQHandler();
int main(void)
{
        // Initialize TIMER
        NRF_TIMER1->MODE = TIMER_MODE_MODE_Timer << TIMER_MODE_MODE_Pos;
        NRF_TIMER1->BITMODE = TIMER_BITMODE_BITMODE_16Bit << TIMER_BITMODE_BITMODE_Pos;
        NRF_TIMER1->SHORTS = TIMER_SHORTS_COMPARE0_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos;
        //NRF_TIMER1->INTENSET = TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos;
        NRF_TIMER1->CC[0] = 1600;
        NRF_TIMER1->PRESCALER = 0;
        NRF_TIMER1->TASKS_START = 1;
        //IRQ_DIRECT_CONNECT(TIMER1_IRQn, 0, TIMER1_IRQHandler, 0);
        //irq_enable(TIMER1_IRQn);

        // Initialize PPI
        NRF_PPI->CH[0].EEP = (uint32_t)&NRF_TIMER1->EVENTS_COMPARE[0];
        NRF_PPI->CH[0].TEP = (uint32_t)&NRF_SAADC->TASKS_START;
        NRF_PPI->FORK[0].TEP = (uint32_t)&NRF_SAADC->TASKS_SAMPLE;
        NRF_PPI->CHENSET = (1 << 0);

        // Initialize SAADC
        NRF_SAADC->RESOLUTION = SAADC_RESOLUTION_VAL_14bit << SAADC_RESOLUTION_VAL_Pos;
        NRF_SAADC->SAMPLERATE = SAADC_SAMPLERATE_MODE_Task << SAADC_SAMPLERATE_MODE_Pos;
        NRF_SAADC->SAMPLERATE |= 1600 << SAADC_SAMPLERATE_CC_Pos;
        NRF_SAADC->CH[0].PSELP = 0x03;  //      AIN3
        NRF_SAADC->CH[0].PSELN = 0;
        NRF_SAADC->CH[0].CONFIG = SAADC_CH_CONFIG_GAIN_Gain1_6 << SAADC_CH_CONFIG_GAIN_Pos;
        NRF_SAADC->CH[0].CONFIG |= SAADC_CH_CONFIG_TACQ_40us << SAADC_CH_CONFIG_TACQ_Pos;
        NRF_SAADC->CH[1].PSELP = 0x04;  //      AIN2
        NRF_SAADC->CH[1].PSELN = 0;
        NRF_SAADC->CH[1].CONFIG = SAADC_CH_CONFIG_GAIN_Gain1_6 << SAADC_CH_CONFIG_GAIN_Pos;
        NRF_SAADC->CH[1].CONFIG |= SAADC_CH_CONFIG_TACQ_40us << SAADC_CH_CONFIG_TACQ_Pos;
        NRF_SAADC->RESULT.PTR = (uint32_t)saadc_buffer;
        NRF_SAADC->RESULT.MAXCNT = 2;
        NRF_SAADC->ENABLE = 0x01;
        NRF_SAADC->TASKS_START = 1;
        NRF_SAADC->TASKS_CALIBRATEOFFSET = 1;
        while(!NRF_SAADC->EVENTS_CALIBRATEDONE){}
        printk("Calibration done.\n");
        NRF_SAADC->TASKS_SAMPLE = 1;

        // Initialize GPIOTE
        NRF_GPIO->PIN_CNF[JOYSTICK_SWITCH] = 0x0C;
        NRF_GPIOTE->CONFIG[0] = GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos;
        NRF_GPIOTE->CONFIG[0] |= JOYSTICK_SWITCH << GPIOTE_CONFIG_PSEL_Pos;
        NRF_GPIOTE->CONFIG[0] |= GPIOTE_CONFIG_POLARITY_HiToLo << GPIOTE_CONFIG_POLARITY_Pos;
        NRF_GPIOTE->INTENSET = GPIOTE_INTENSET_IN0_Enabled << GPIOTE_INTENSET_IN0_Pos;
        IRQ_DIRECT_CONNECT(GPIOTE_IRQn, 0, GPIOTE_IRQHandler, 0);
        irq_enable(GPIOTE_IRQn);
        
        while(true){
                for(uint8_t i=0;i<2;i++){
                        if(maximum[i] < saadc_buffer[i]){
                                maximum[i] = saadc_buffer[i];
                        }else if(minimum[i] > saadc_buffer[i]){
                                minimum[i] = saadc_buffer[i];
                        }
                }
                printk("X-Axis : %d, %d, %.2f\t", maximum[0], minimum[0], (float)(saadc_buffer[0] - minimum[0])/(float)(maximum[0] - minimum[0]));
                printk("Y-Axis : %d, %d, %.2f\n", maximum[1], minimum[1], (float)(saadc_buffer[1] - minimum[1])/(float)(maximum[1] - minimum[1]));
        }
        return 0;
}

void GPIOTE_IRQHandler(){
        // IRQ Handler for GPIOTE
        if(NRF_GPIOTE->EVENTS_IN[0] == 1){
                // Joystick Switch Events
                NRF_GPIOTE->EVENTS_IN[0] = 0;
                printk("TEST\n");
        }
}

void TIMER1_IRQHandler(){
        // 타이머 핸들러
        if(NRF_TIMER1->EVENTS_COMPARE[0] == 1){
                NRF_TIMER1->EVENTS_COMPARE[0] = 0;
                //NRF_SAADC->TASKS_START = 1;
                NRF_SAADC->TASKS_SAMPLE = 1;
        }
}