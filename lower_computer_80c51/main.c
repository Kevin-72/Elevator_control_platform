#include <reg51.h>

#define uchar unsigned char

// ���ڽ��ջ�����
uchar received_bytes[20];
uchar received_count = 0;

// �����жϷ�����
void serial_isr() interrupt 4 {
    if (RI) {  // �����ж�
        RI = 0;  // ������ձ�־
        received_bytes[received_count++] = SBUF;  // �����յ��ֽڴ��뻺����
    }
}

// ���ڳ�ʼ������
void uart_init() {
    TMOD = 0x20;  // ��ʱ��1��ģʽ2
    TH1 = 0xFD;   // ������9600
    TL1 = 0xFD;
    TR1 = 1;      // ������ʱ��1
    SCON = 0x50;  // ����ģʽ1���������
    ES = 1;       // ���������ж�
    EA = 1;       // �������ж�
}

// ���ڷ��ͺ���
void send_response(uchar *response_bytes, uchar response_length) {
    uchar i;
    for (i = 0; i < response_length; i++) {
        SBUF = response_bytes[i];
        while (!TI);  // �ȴ��������
        TI = 0;       // ���������ɱ�־
    }
}

// ������
void main() {
    uchar command1[] = {0x55, 0xAA, 0x00, 0x00, 0x00, 0x00, 0xFF};
    uchar response1[] = {0x55, 0xAA, 0x03, 0x00, 0x00, 0x01, 0x01, 0x04};
    uchar command2[] = {0x55, 0xAA, 0x00, 0x08, 0x00, 0x00, 0x07};
    uchar response2[] = {0x55, 0xAA, 0x03, 0x07, 0x00, 0x0C, 0x69, 0x02, 0x00, 0x08, 
                         0x01, 0x01, 0x11, 0x22, 0x10, 0x33, 0x02, 0x01, 0x03};

    uart_init();  // ��ʼ������

    while (1) {
        if (received_count == 7) {  // ����Ƿ���յ���������1������2
            uchar is_match1 = 1;
            uchar is_match2 = 1;
            uchar i;

            // ��֤�Ƿ��� command1 ƥ��
            for (i = 0; i < 7; i++) {
                if (received_bytes[i] != command1[i]) {
                    is_match1 = 0;
                }
                if (received_bytes[i] != command2[i]) {
                    is_match2 = 0;
                }
            }

            if (is_match1) {  // ���ƥ������1
                send_response(response1, sizeof(response1));
            } else if (is_match2) {  // ���ƥ������2
                send_response(response2, sizeof(response2));
            }

            received_count = 0;  // ���ý��ռ�����׼����һ�ν���
        }
    }
}
