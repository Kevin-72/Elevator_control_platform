#include <reg51.h>

#define uchar unsigned char

// 串口接收缓冲区
uchar received_bytes[20];
uchar received_count = 0;

// 串口中断服务函数
void serial_isr() interrupt 4 {
    if (RI) {  // 接收中断
        RI = 0;  // 清除接收标志
        received_bytes[received_count++] = SBUF;  // 将接收的字节存入缓冲区
    }
}

// 串口初始化函数
void uart_init() {
    TMOD = 0x20;  // 定时器1，模式2
    TH1 = 0xFD;   // 波特率9600
    TL1 = 0xFD;
    TR1 = 1;      // 启动定时器1
    SCON = 0x50;  // 串口模式1，允许接收
    ES = 1;       // 开启串口中断
    EA = 1;       // 开启总中断
}

// 串口发送函数
void send_response(uchar *response_bytes, uchar response_length) {
    uchar i;
    for (i = 0; i < response_length; i++) {
        SBUF = response_bytes[i];
        while (!TI);  // 等待发送完成
        TI = 0;       // 清除发送完成标志
    }
}

// 主函数
void main() {
    uchar command1[] = {0x55, 0xAA, 0x00, 0x00, 0x00, 0x00, 0xFF};
    uchar response1[] = {0x55, 0xAA, 0x03, 0x00, 0x00, 0x01, 0x01, 0x04};
    uchar command2[] = {0x55, 0xAA, 0x00, 0x08, 0x00, 0x00, 0x07};
    uchar response2[] = {0x55, 0xAA, 0x03, 0x07, 0x00, 0x0C, 0x69, 0x02, 0x00, 0x08, 
                         0x01, 0x01, 0x11, 0x22, 0x10, 0x33, 0x02, 0x01, 0x03};

    uart_init();  // 初始化串口

    while (1) {
        if (received_count == 7) {  // 检查是否接收到完整命令1或命令2
            uchar is_match1 = 1;
            uchar is_match2 = 1;
            uchar i;

            // 验证是否与 command1 匹配
            for (i = 0; i < 7; i++) {
                if (received_bytes[i] != command1[i]) {
                    is_match1 = 0;
                }
                if (received_bytes[i] != command2[i]) {
                    is_match2 = 0;
                }
            }

            if (is_match1) {  // 如果匹配命令1
                send_response(response1, sizeof(response1));
            } else if (is_match2) {  // 如果匹配命令2
                send_response(response2, sizeof(response2));
            }

            received_count = 0;  // 重置接收计数，准备下一次接收
        }
    }
}
