#ifndef __LIVE_H265_BUFFER_TYPE_HH
#define __LIVE_H265_BUFFER_TYPE_HH

#ifdef __cplusplus
extern "C"{
#endif

#define TRANSPORT_SYNC_BYTE 0x47        // TS��ͬ���ֽ�
#define REC_BUF_MAX_LEN 256*1024+2*188  // ÿ��buffer����Ч���ݳ���Ϊ256K�ֽ�,�����2*188�ֽڴ��PAT���PMT��

typedef struct s_buffer 
{
    unsigned char   buf_writing;        // buffer�Ƿ��ڱ�д״̬��buffer��д��״̬���ȶ�����Ҫ�ȴ�д��ɲ��ܶ�ȡ
    unsigned int    buf_len;            // д�����ݵ�ʵ�ʳ���
    unsigned int    buf_read_counter;   // ��¼�Ѿ���ȡ���ֽ���
    unsigned char   buf_data[REC_BUF_MAX_LEN];  // ������ݵ�buffer
} t_h265_Buffer;

#ifdef __cplusplus
}
#endif


#endif




