#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "inc/hw_i2c.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "driverlib/gpio.h"
#include "driverlib/i2c.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "driverlib/debug.h"
#include "driverlib/interrupt.h"
#include "driverlib/adc.h"
#include "inc/hw_ints.h"
#include "driverlib/timer.h"
#include "driverlib/pwm.h"
#include "driverlib/rom_map.h"

//https://www.digikey.com/eewiki/display/microcontroller/I2C+Communication+with+the+TI+Tiva+TM4C123GXL
//! - I2C0SCL - PB2
//! - I2C0SDA - PB3

#define NUM_I2C_DATA 2
#define SLAVE_ADDRESS 0x3C
uint8_t g_master_buff[NUM_I2C_DATA];
unsigned char LCD_screen_buffer[1024];
char LCD_line1[21];
char LCD_line2[21];
char LCD_line3[21];

int sensor = 0; // cuenta los hoyitos del encoder
int enable = 0;

void InitI2C0()
{
    // The I2C0 peripheral must be enabled before use.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);
    //reset module
    SysCtlPeripheralReset(SYSCTL_PERIPH_I2C0);
    // For this example I2C0 is used with PortB[3:2].
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    // Configure the pin muxing for I2C0 functions on port B2 and B3.
    GPIOPinConfigure(GPIO_PB2_I2C0SCL);
    GPIOPinConfigure(GPIO_PB3_I2C0SDA);
    // Select the I2C function for these pins.
    GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
    GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);
    // Enable and initialize the I2C0 master module.  Use the system clock for
    // the I2C0 module.  The last parameter sets the I2C data transfer rate.
    // If false the data rate is set to 100kbps and if true the data rate will be set to 400kbps.
    I2CMasterInitExpClk(I2C0_BASE, SysCtlClockGet(), true);
    //clear I2C FIFOs
    HWREG(I2C0_BASE + I2C_O_FIFOCTL) = 80008000;

    I2CMasterSlaveAddrSet(I2C0_BASE, SLAVE_ADDRESS, false);
}

void LCD_send_Command(unsigned char* commands,unsigned int command_length )
{

    int x=0;
    for (x=0; x<command_length; x++)
    {
        I2CMasterDataPut(I2C0_BASE, 0x00);
        I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);
        // Wait until MCU is done transferring.
        while(I2CMasterBusy(I2C0_BASE));
        I2CMasterDataPut(I2C0_BASE, commands[x]) ;
        I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
        while(I2CMasterBusy(I2C0_BASE));
    }
}

void LCD_send_Data(unsigned char* data_, unsigned int data_length)
{
    int x=0;
    for (x=0; x<data_length; x++)
    {
        I2CMasterDataPut(I2C0_BASE, 0x40);
        I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);
        // Wait until MCU is done transferring.
        while(I2CMasterBusy(I2C0_BASE));
        I2CMasterDataPut(I2C0_BASE, data_[x]) ;
        I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
        while(I2CMasterBusy(I2C0_BASE));
    }
}

void LCD_init()
{
    unsigned char init_sequence[27]={0xAE,0xD5,0x80,0xA8,63,0xD3,0x00,0x40,0x8D,
            0xD5,0x14,0x20,0x00,0xA1,0xC8,0xDA,0x12,0x81,0xCF,0xD9,0xF1,  0xDB,0x40,
            0xA4,0xA6,0x2E,0xAF};
    LCD_send_Command(init_sequence,27);
}

void LCD_print()
{
    unsigned char start_screen[6]={0x22,0x00,0xFF,0x21,0x00,127};
    LCD_send_Command(start_screen,6);
    LCD_send_Data(LCD_screen_buffer,1024);
}

void LCD_parse (char* letra, unsigned char caracter)
{
    switch (caracter){
        case 'A':
            letra[0]=0xFC;letra[1]=0x12;letra[2]=0x11;letra[3]=0x11;letra[4]=0xFE;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'B':
            letra[0]=0xFF;letra[1]=0x89;letra[2]=0x89;letra[3]=0x76;letra[4]=0x00;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'C':
            letra[0]=0x7E;letra[1]=0x81;letra[2]=0x81;letra[3]=0x81;letra[4]=0x00;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'D':
            letra[0]=0xFF;letra[1]=0x81;letra[2]=0x81;letra[3]=0x7E;letra[4]=0x00;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'E':
            letra[0]=0xFF;letra[1]=0x89;letra[2]=0x89;letra[3]=0x89;letra[4]=0x81;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'F':
            letra[0]=0xFF;letra[1]=0x09;letra[2]=0x09;letra[3]=0x09;letra[4]=0x01;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'G':
            letra[0]=0x7E;letra[1]=0x81;letra[2]=0x91;letra[3]=0x91;letra[4]=0x72;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'H':
            letra[0]=0xFF;letra[1]=0x08;letra[2]=0x08;letra[3]=0x08;letra[4]=0xFF;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'I':
            letra[0]=0x81;letra[1]=0x81;letra[2]=0xFF;letra[3]=0x81;letra[4]=0x81;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'J':
            letra[0]=0x70;letra[1]=0x80;letra[2]=0x81;letra[3]=0x81;letra[4]=0x7F;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'K':
            letra[0]=0xFF;letra[1]=0x08;letra[2]=0x14;letra[3]=0x22;letra[4]=0xC1;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'L':
            letra[0]=0xFF;letra[1]=0x80;letra[2]=0x80;letra[3]=0x80;letra[4]=0x80;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'M':
            letra[0]=0xFF;letra[1]=0x06;letra[2]=0x0C;letra[3]=0x06;letra[4]=0xFF;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'N':
            letra[0]=0xFF;letra[1]=0x06;letra[2]=0x18;letra[3]=0x60;letra[4]=0xFF;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'O':
            letra[0]=0x7E;letra[1]=0x81;letra[2]=0x81;letra[3]=0x81;letra[4]=0x7E;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'P':
            letra[0]=0xFF;letra[1]=0x09;letra[2]=0x09;letra[3]=0x09;letra[4]=0x06;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'Q':
            letra[0]=0x7E;letra[1]=0x81;letra[2]=0xA1;letra[3]=0x7E;letra[4]=0x80;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'R':
            letra[0]=0xFF;letra[1]=0x09;letra[2]=0x09;letra[3]=0x09;letra[4]=0xF6;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'S':
            letra[0]=0x4E;letra[1]=0x89;letra[2]=0x89;letra[3]=0x89;letra[4]=0x72;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'T':
            letra[0]=0x01;letra[1]=0x01;letra[2]=0xFF;letra[3]=0x01;letra[4]=0x01;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'U':
            letra[0]=0x7F;letra[1]=0x80;letra[2]=0x80;letra[3]=0x80;letra[4]=0x7F;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'V':
            letra[0]=0x0F;letra[1]=0x70;letra[2]=0x80;letra[3]=0x70;letra[4]=0x0F;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'W':
            letra[0]=0x3F;letra[1]=0xC0;letra[2]=0x30;letra[3]=0xC0;letra[4]=0x3F;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'X':
            letra[0]=0xC3;letra[1]=0x24;letra[2]=0x18;letra[3]=0x24;letra[4]=0xC3;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'Y':
            letra[0]=0x03;letra[1]=0x0C;letra[2]=0xF0;letra[3]=0x0C;letra[4]=0x03;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'Z':
            letra[0]=0xC1;letra[1]=0xA1;letra[2]=0x99;letra[3]=0x85;letra[4]=0x83;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'a':
            letra[0]=0x64;letra[1]=0x94;letra[2]=0x94;letra[3]=0x94;letra[4]=0xF8;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'b':
            letra[0]=0xFE;letra[1]=0x90;letra[2]=0x90;letra[3]=0x90;letra[4]=0x60;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'c':
            letra[0]=0x70;letra[1]=0x88;letra[2]=0x88;letra[3]=0x88;letra[4]=0x88;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'd':
            letra[0]=0x60;letra[1]=0x90;letra[2]=0x90;letra[3]=0x90;letra[4]=0xFE;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'e':
            letra[0]=0x70;letra[1]=0xA8;letra[2]=0xA8;letra[3]=0xA8;letra[4]=0xB0;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'f':
            letra[0]=0xFC;letra[1]=0x12;letra[2]=0x12;letra[3]=0x04;letra[4]=0x00;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'g':
            letra[0]=0xCC;letra[1]=0x8A;letra[2]=0x8A;letra[3]=0x8A;letra[4]=0x7C;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'h':
            letra[0]=0xFE;letra[1]=0x10;letra[2]=0x10;letra[3]=0x10;letra[4]=0xE0;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'i':
            letra[0]=0x00;letra[1]=0x00;letra[2]=0xF2;letra[3]=0x00;letra[4]=0x00;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'j':
            letra[0]=0x60;letra[1]=0x80;letra[2]=0x80;letra[3]=0x80;letra[4]=0xFA;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'k':
            letra[0]=0xFE;letra[1]=0x10;letra[2]=0x28;letra[3]=0x44;letra[4]=0x80;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'l':
            letra[0]=0x00;letra[1]=0x00;letra[2]=0xFE;letra[3]=0x00;letra[4]=0x00;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'm':
            letra[0]=0xF8;letra[1]=0x08;letra[2]=0xF0;letra[3]=0x08;letra[4]=0xF0;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'n':
            letra[0]=0xF8;letra[1]=0x08;letra[2]=0x08;letra[3]=0x08;letra[4]=0xF0;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'o':
            letra[0]=0x70;letra[1]=0x88;letra[2]=0x88;letra[3]=0x88;letra[4]=0x70;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'p':
            letra[0]=0xFC;letra[1]=0x14;letra[2]=0x14;letra[3]=0x14;letra[4]=0x08;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'q':
            letra[0]=0x06;letra[1]=0x09;letra[2]=0x29;letra[3]=0xFE;letra[4]=0x00;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'r':
            letra[0]=0xFC;letra[1]=0x08;letra[2]=0x04;letra[3]=0x04;letra[4]=0x00;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 's':
            letra[0]=0x8C;letra[1]=0x92;letra[2]=0x92;letra[3]=0x62;letra[4]=0x00;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 't':
            letra[0]=0x08;letra[1]=0xFE;letra[2]=0x88;letra[3]=0x00;letra[4]=0x00;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'u':
            letra[0]=0x7C;letra[1]=0x80;letra[2]=0x80;letra[3]=0xFC;letra[4]=0x00;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'v':
            letra[0]=0x3C;letra[1]=0x48;letra[2]=0x80;letra[3]=0x48;letra[4]=0x3C;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'w':
            letra[0]=0x3C;letra[1]=0xC0;letra[2]=0x20;letra[3]=0xC0;letra[4]=0x3C;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'x':
            letra[0]=0xCC;letra[1]=0x30;letra[2]=0x30;letra[3]=0xCC;letra[4]=0x00;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'y':
            letra[0]=0x4C;letra[1]=0x90;letra[2]=0x90;letra[3]=0x7C;letra[4]=0x00;letra[5]=0x00;//letra[6]=0x00;
            break;

        case 'z':
            letra[0]=0xC4;letra[1]=0xA4;letra[2]=0xB4;letra[3]=0x94;letra[4]=0x8C;letra[5]=0x00;//letra[6]=0x00;
            break;

        case '1':
            letra[0]=0x84;letra[1]=0x82;letra[2]=0xFF;letra[3]=0x80;letra[4]=0x80;letra[5]=0x00;//letra[6]=0x00;
            break;

        case '2':
            letra[0]=0xC2;letra[1]=0xA1;letra[2]=0x91;letra[3]=0x91;letra[4]=0x8E;letra[5]=0x00;//letra[6]=0x00;
            break;

        case '3':
            letra[0]=0x42;letra[1]=0x81;letra[2]=0x99;letra[3]=0x99;letra[4]=0x66;letra[5]=0x00;//letra[6]=0x00;
            break;

        case '4':
            letra[0]=0x18;letra[1]=0x14;letra[2]=0x12;letra[3]=0xFF;letra[4]=0x10;letra[5]=0x00;//letra[6]=0x00;
            break;

        case '5':
            letra[0]=0x4F;letra[1]=0x89;letra[2]=0x89;letra[3]=0x89;letra[4]=0x71;letra[5]=0x00;//letra[6]=0x00;
            break;

        case '6':
            letra[0]=0x7E;letra[1]=0x89;letra[2]=0x89;letra[3]=0x89;letra[4]=0x72;letra[5]=0x00;//letra[6]=0x00;
            break;

        case '7':
            letra[0]=0x01;letra[1]=0xF1;letra[2]=0x09;letra[3]=0x05;letra[4]=0x03;letra[5]=0x00;//letra[6]=0x00;
            break;

        case '8':
            letra[0]=0x76;letra[1]=0x89;letra[2]=0x89;letra[3]=0x89;letra[4]=0x76;letra[5]=0x00;//letra[6]=0x00;
            break;

        case '9':
            letra[0]=0x4E;letra[1]=0x91;letra[2]=0x91;letra[3]=0x91;letra[4]=0x7E;letra[5]=0x00;//letra[6]=0x00;
            break;

        case '0':
            letra[0]=0x7E;letra[1]=0xA1;letra[2]=0x99;letra[3]=0x85;letra[4]=0x7E;letra[5]=0x00;//letra[6]=0x00;
            break;

        case ' ':
            letra[0]=0x00;letra[1]=0x00;letra[2]=0x00;letra[3]=0x00;letra[4]=0x00;letra[5]=0x00;//letra[6]=0x00;
            break;

        default:
            letra[0]=0x00;letra[1]=0x00;letra[2]=0x00;letra[3]=0x00;letra[4]=0x00;letra[5]=0x00;//letra[6]=0x00;
            break;
    }

}

void LCD_print_text()
{
    int y,z;

    char buffer_letra[6];
    for(y=0; y<21 ;y++)
    {
        LCD_parse(buffer_letra, LCD_line1[y]);
        for(z=0; z<6 ;z++)
        {
            LCD_screen_buffer[130+ (6*y) + z] = buffer_letra[0 + z];
        }
    }
    for(y=0; y<21 ;y++)
    {
        LCD_parse(buffer_letra, LCD_line2[y]);
        for(z=0; z<6 ;z++)
        {
            LCD_screen_buffer[386+ (6*y) + z] = buffer_letra[0 + z];
        }
    }
    for(y=0; y<21 ;y++)
    {
        LCD_parse(buffer_letra, LCD_line3[y]);
        for(z=0; z<6 ;z++)
        {
            LCD_screen_buffer[642+ (6*y) + z] = buffer_letra[0 + z];
        }
    }

    LCD_print();
}

void LCD_clear()
{
    memset(&LCD_screen_buffer, 0, 1024);
    LCD_print();
}
void LCD_set()
{
    memset(&LCD_screen_buffer, 0xFF, 1024);
    LCD_print();
}

// Timer
static volatile uint32_t Counter = 0;

void Timer0BIntHandler(void)
{
    Counter++;
    TimerIntClear(TIMER0_BASE, TIMER_TIMB_TIMEOUT);
}

// Interrupcion
void GPIOF_Handler_mifuncion()
{
    uint32_t banderas;

    banderas = GPIOIntStatus(GPIO_PORTF_BASE, true);

    GPIOIntClear(GPIO_PORTF_BASE, GPIO_INT_PIN_4);

    banderas = GPIOIntStatus(GPIO_PORTF_BASE, true);

    sensor++;
    /*if (enable == 0){
        enable = 1;
    }
    else
    {
        enable = 0;
        sensor++;
    }*/

}

void main(void)
{
    uint32_t valorADC;
    int PWMPeriod1 = 0;
    int Vel = 0; // Guarda la velocidad a la que se desea llegar
    int vueltas = 0; // Cuenta las vueltas que ha dado el encoder
    int RPMA = 0; //RPM Actuales
    int RPMD = 350; //RPM Deseadas
    int Start = 0;

    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    // inicializar el I2C
    InitI2C0();
    LCD_init();
    LCD_clear();

    // Configurar el ADC
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    GPIOPinTypeADC(GPIO_PORTE_BASE,GPIO_PIN_2);
    ADCSequenceConfigure(ADC0_BASE, 2, ADC_TRIGGER_PROCESSOR, 0); // el 2 es el secuenciador a utilizar (SS2)
    ADCSequenceStepConfigure(ADC0_BASE, 2, 0, ADC_CTL_CH0 | ADC_CTL_IE | ADC_CTL_END); // el CH son los canales a utilizar
    ADCSequenceEnable(ADC0_BASE, 2); // el 2 es el secuenciador a utilizar (SS2)
    ADCIntClear(ADC0_BASE, 2); // el 2 es el secuenciador a utilizar (SS2)

    // Boton Start
    GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_4); // Pin E4

    // Configurar PWM
    SysCtlPWMClockSet(SYSCTL_PWMDIV_1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinConfigure(GPIO_PF1_M1PWM5); // Puerto F1
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1);
    PWMGenConfigure(PWM1_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_2, 1000);
    PWMPeriod1 = PWMGenPeriodGet(PWM1_BASE, PWM_GEN_2);
    //PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, PWMPeriod1 / 4);
    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, 1);
    PWMOutputState(PWM1_BASE, PWM_OUT_5_BIT , true);
    PWMGenEnable(PWM1_BASE, PWM_GEN_2);

    // Interrupcion
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_3 | GPIO_PIN_4); // Pin F3 -> Boton Stop; Pin F4 -> Leer el valor del sensor de herradura
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPD);
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPD);
    //GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_RISING_EDGE);
    GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_RISING_EDGE);
    GPIOIntRegister(GPIO_PORTF_BASE, GPIOF_Handler_mifuncion);
    //GPIOIntEnable(GPIO_PORTF_BASE, GPIO_INT_PIN_3);
    GPIOIntEnable(GPIO_PORTF_BASE, GPIO_INT_PIN_4);

    // Configurar el Timer
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_B_PERIODIC);
    TimerLoadSet(TIMER0_BASE, TIMER_B, SysCtlClockGet() / 1000); // Timer a 1ms
    IntMasterEnable();
    TimerIntEnable(TIMER0_BASE, TIMER_TIMB_TIMEOUT);
    IntEnable(INT_TIMER0B);
    TimerEnable(TIMER0_BASE, TIMER_B);

    while(1)
    {
        // Lectura del ADC
        ADCProcessorTrigger(ADC0_BASE, 2); // el 2 es el secuenciador a utilizar (SS2)
        while(!ADCIntStatus(ADC0_BASE, 2, false));
        ADCIntClear(ADC0_BASE, 2);
        ADCSequenceDataGet(ADC0_BASE, 2, &valorADC);
        SysCtlDelay(SysCtlClockGet() / 12);

        Vel = (valorADC*13000)/4095;
        if(Vel<100)
        {
            Vel=50;
        }

        // I2C LCD
        LCD_clear();
        sprintf(LCD_line1,"      Velocidad      ");
        sprintf(LCD_line2,"         %d          ",Vel); // Se muestra la velocidad a la que se quiere llegar con el pot
        sprintf(LCD_line3,"                     ");
        LCD_print_text();
        SysCtlDelay(SysCtlClockGet()/5);

        Start = GPIOPinRead(GPIO_PORTE_BASE, GPIO_PIN_4);

        if(Start==0x10) // Si se presiona el boton start
        {
            IntMasterEnable();
            while(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_3)==0)
            {
                //GPIOF_Handler_mifuncion();

                PWMOutputState(PWM1_BASE, PWM_OUT_5_BIT , true);
                PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, RPMD);

                Counter=0;
                //sensor=0;
                while(Counter<=100) // Cuenta cada 1 segundo
                {
                    if(sensor==20) // 1 vuelta del encoder = 1 revolución
                    {
                        vueltas++;
                        sensor=0;
                    }
                    else if(sensor>20)
                    {
                        sensor=0;
                    }
                }
                RPMA = vueltas*60; // Multiplica las vueltas por 60 para obtener las revoluciones por minuto
                vueltas=0;

                if(RPMA==Vel)
                {
                    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, RPMD);
                }
                else if(RPMA<Vel)
                {
                    RPMD=RPMD+10;
                }
                else if(RPMA>Vel)
                {
                    RPMD=RPMD-10;
                }

                LCD_clear();
                sprintf(LCD_line1," RPM deseadas   %d   ",Vel); // Se muestran las RPM Deseadas
                sprintf(LCD_line2," RPM actuales   %d   ",RPMA); // Se muestran las RPM Actuales
                sprintf(LCD_line3," Valor PWM %d        ",PWMPulseWidthGet(PWM1_BASE, PWM_OUT_5)); // Se muestra el valor actual del PWM
                LCD_print_text();
                //SysCtlDelay(SysCtlClockGet()/5);
            }
            PWMOutputState(PWM1_BASE, PWM_OUT_5_BIT , false);
            IntMasterDisable();
        }
        else
        {
             // Nada
        }

    }

}

// El encoder tiene 20 hoyos
