#include <reg51.h>
#include <string.h>  // �����ַ�����������

typedef void(*pFunc)();
pFunc func=NULL;

#define BUFFER_SIZE 100

char buffer[BUFFER_SIZE]; // ���ջ�����
unsigned char buffer_index = 0;  // ����������
int stopFlag = 1;
sbit RS485E=P3^7;   //����485��ʹ�ܽ�
//bit SendFlag;
//unsigned int ReData,SenData;


void Delay10ms(unsigned int c)   //��� 0us
{
    unsigned char a, b;

	//--c�Ѿ��ڴ��ݹ�����ʱ���Ѿ���ֵ�ˣ�������for����һ��Ͳ��ø�ֵ��--//
    for (;c>0;c--)
	{
		for (b=38;b>0;b--)
		{
			for (a=130;a>0;a--);
		}         
	}
}

/* ���ڳ�ʼ������ */
void SerialInit(void) {
    TMOD = 0x20;      // ��ʱ��1ģʽ2��8λ�Զ���װ��
    TH1 = 0xFD;       // ������9600, Fosc=11.0592MHz
    SCON = 0x50;      // ģʽ1, �����������
    TR1 = 1;          // ������ʱ��1
    TI = 0;           // ������ͱ�־
    RI = 0;           // ������ձ�־
    ES = 1;           // ʹ�ܴ����ж�
    EA = 1;           // ȫ���ж�ʹ��
}


void up()
{
	stopFlag = 0;
	while(1)	
	{
		P0 = 0xff;
		
		if(stopFlag)
		{
			break;
		}
	}
}

void down()
{
	stopFlag = 0;
	while(1)	
	{
		P0 = 0x55;
		
		if(stopFlag)
		{
			break;
		}
	}
}

void stop()
{
	stopFlag = 0;
	while(1)
	{
		P0 = 0;
		if(stopFlag)
		{
			break;
		}
	}
}

void mode01()
{
	unsigned char LED = ~1;
	stopFlag = 0;
	while(1)	
	{
		P0 = LED;
		LED = LED << 1;
		if (LED == 0)
		{
			LED = ~1;	
		}	
		Delay10ms(50);
		
		if(stopFlag)
		{
			break;
		}
	}
}

void mode02()
{
	unsigned char LED = 1;
	stopFlag = 0;
	while(1)	
	{
		P0 = LED;
		LED = LED << 1;
		if (LED == 0)
		{
			LED = 1;	
		}	
		Delay10ms(50);
		
		if(stopFlag)
		{
			break;
		}
	}
}

void mode03()
{
	unsigned char LED = 0xf;
	stopFlag = 0;
	while(1)	
	{
		P0 = LED;
		LED = ~LED;	
		Delay10ms(50);
		
		if(stopFlag)
		{
			break;
		}
	}
}

void mode04()
{
	unsigned char LED = 0x55;
	stopFlag = 0;
	while(1)	
	{
		P0 = LED;
		LED = ~LED;	
		Delay10ms(50);
		
		if(stopFlag)
		{
			break;
		}
	}
}

void mode05()
{
	unsigned char LED = 0x33;
	stopFlag = 0;
	while(1)	
	{
		P0 = LED;
		LED = ~LED;	
		Delay10ms(50);
		
		if(stopFlag)
		{
			break;
		}
	}
}

/* ������յ������� */
void ProcessCommand(char *cmd) {
	stopFlag = 1;

    if (strcmp(cmd, "CMD=UP\r\n") == 0) {
        func=up;
    } else if (strcmp(cmd, "CMD=DOWN\r\n") == 0) {
        func=down;
	} else if (strcmp(cmd, "CMD=STOP\r\n") == 0) {
		func=stop;
	} else if (strcmp(cmd, "MODE=01\r\n") == 0) {
		func = mode01;
	} else if (strcmp(cmd, "MODE=02\r\n") == 0) {
		func = mode02;
	} else if (strcmp(cmd, "MODE=03\r\n") == 0) {
		func = mode03;
	} else if (strcmp(cmd, "MODE=04\r\n") == 0) {
		func = mode04;
	} else if (strcmp(cmd, "MODE=05\r\n") == 0) {
		func = mode05;
    }


    memset(buffer, 0, BUFFER_SIZE);  // ��ջ�����
}

/* ���ڷ����ַ����� */
void SerialSend(char dat) {
    SBUF = dat;       // �����ݷŵ����ڻ���Ĵ���
    while (!TI);      // �ȴ��������
    TI = 0;           // ���������ɱ�־
}

/* �����жϷ������� */
void Serial_ISR(void) interrupt 4 {
    if (RI) {
        char received_char = SBUF;
        RI = 0;  // ��������жϱ�־
        buffer[buffer_index++] = received_char;  // �洢���յ����ַ�
        if (received_char == '\n' && buffer_index > 1 && buffer[buffer_index-2] == '\r') {
            buffer[buffer_index] = '\0';  // ȷ���ַ�������
            ProcessCommand(buffer);  // ������յ�������
            buffer_index = 0;  // ���û���������
        }
        if (buffer_index >= BUFFER_SIZE - 1) {
            buffer_index = 0;  // ��ֹ���������
        }
    }
}

/* ������ */
int main(void) {
    SerialInit();  // ��ʼ������
	RS485E=0; 

	func = stop;
    while (1) {
        func();
		//P0 = 0xF2;
		//Delay10ms(100);
    }
}
