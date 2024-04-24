#include <reg51.h>
#include <string.h>  // 引入字符串操作函数

typedef void(*pFunc)();
pFunc func=NULL;

#define BUFFER_SIZE 100

char buffer[BUFFER_SIZE]; // 接收缓冲区
unsigned char buffer_index = 0;  // 缓冲区索引
int stopFlag = 1;
sbit RS485E=P3^7;   //定义485的使能脚
//bit SendFlag;
//unsigned int ReData,SenData;


void Delay10ms(unsigned int c)   //误差 0us
{
    unsigned char a, b;

	//--c已经在传递过来的时候已经赋值了，所以在for语句第一句就不用赋值了--//
    for (;c>0;c--)
	{
		for (b=38;b>0;b--)
		{
			for (a=130;a>0;a--);
		}         
	}
}

/* 串口初始化函数 */
void SerialInit(void) {
    TMOD = 0x20;      // 定时器1模式2，8位自动重装载
    TH1 = 0xFD;       // 波特率9600, Fosc=11.0592MHz
    SCON = 0x50;      // 模式1, 串口允许接收
    TR1 = 1;          // 启动定时器1
    TI = 0;           // 清除发送标志
    RI = 0;           // 清除接收标志
    ES = 1;           // 使能串口中断
    EA = 1;           // 全局中断使能
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

/* 处理接收到的命令 */
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


    memset(buffer, 0, BUFFER_SIZE);  // 清空缓冲区
}

/* 串口发送字符函数 */
void SerialSend(char dat) {
    SBUF = dat;       // 将数据放到串口缓冲寄存器
    while (!TI);      // 等待发送完成
    TI = 0;           // 清除发送完成标志
}

/* 串口中断服务例程 */
void Serial_ISR(void) interrupt 4 {
    if (RI) {
        char received_char = SBUF;
        RI = 0;  // 清除接收中断标志
        buffer[buffer_index++] = received_char;  // 存储接收到的字符
        if (received_char == '\n' && buffer_index > 1 && buffer[buffer_index-2] == '\r') {
            buffer[buffer_index] = '\0';  // 确保字符串结束
            ProcessCommand(buffer);  // 处理接收到的命令
            buffer_index = 0;  // 重置缓冲区索引
        }
        if (buffer_index >= BUFFER_SIZE - 1) {
            buffer_index = 0;  // 防止缓冲区溢出
        }
    }
}

/* 主函数 */
int main(void) {
    SerialInit();  // 初始化串口
	RS485E=0; 

	func = stop;
    while (1) {
        func();
		//P0 = 0xF2;
		//Delay10ms(100);
    }
}
