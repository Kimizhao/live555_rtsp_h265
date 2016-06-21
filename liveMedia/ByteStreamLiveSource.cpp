
#include "ByteStreamLiveSource.hh"
#include "GroupsockHelper.hh"

////////// ByteStreamLiveSource //////////

#include "LiveH265Type.hh"

ByteStreamLiveSource*
ByteStreamLiveSource::createNew(UsageEnvironment& env, 
                t_h265_Buffer *p_tsbuf,
				unsigned preferredFrameSize,
				unsigned playTimePerFrame)
{
    printf("=== zyl ===, %s, %d\n",__FILE__, __LINE__);

    if (p_tsbuf == NULL) 
        return NULL;

    ByteStreamLiveSource* newSource
        = new ByteStreamLiveSource(env, p_tsbuf, preferredFrameSize, playTimePerFrame);
    //newSource->fPTsBuf = p_tsbuf;
    // ��ʼ��
    newSource->fLocalBuf.buf_writing = 0;
    newSource->fLocalBuf.buf_len = 0;
    newSource->fLocalBuf.buf_read_counter = 0;
    memset(newSource->fLocalBuf.buf_data, 0, REC_BUF_MAX_LEN);

    return newSource;
}

ByteStreamLiveSource::ByteStreamLiveSource(UsageEnvironment& env, 
                        t_h265_Buffer *p_tsbuf,
                        unsigned preferredFrameSize,
                        unsigned playTimePerFrame)
  : FramedSource(env), fPTsBuf(p_tsbuf),fPreferredFrameSize(preferredFrameSize),
    fPlayTimePerFrame(playTimePerFrame), fLastPlayTime(0),
    fHaveStartedReading(False), fLimitNumBytesToStream(False), fNumBytesToStream(0) 
{

}

ByteStreamLiveSource::~ByteStreamLiveSource() 
{

}

void ByteStreamLiveSource::doGetNextFrame() 
{
    if (fLimitNumBytesToStream && fNumBytesToStream == 0) 
    {
        handleClosure();
        return;
    }

    doReadFromBuffer();
}

void ByteStreamLiveSource::doStopGettingFrames() 
{
    envir().taskScheduler().unscheduleDelayedTask(nextTask());
}

void ByteStreamLiveSource::doReadFromBuffer() 
{
#if 1
    //printf("=== zyl ===, %s, %d\n",__FILE__, __LINE__);
    //��ʼ��������
    unsigned int readLen = 0; 
    unsigned int syncBytePosition = 0;

    // û�ã�֮ǰȷ������ȡ�ֽ����ı���
    fMaxSize = fPreferredFrameSize;

    //printf("=== zyl ===, fLocalBuf.buf_read_counter = %d, fLocalBuf.buf_len = %d\n",
    //            fLocalBuf.buf_read_counter, fLocalBuf.buf_len);
    //��ʼ��Frame Size 
    fFrameSize = 0;

    //���ʣ����ֽ����������ȶ�ȡʣ����ֽ���
    if((fLocalBuf.buf_len - fLocalBuf.buf_read_counter) < fPreferredFrameSize)
    {
        // fMaxSize = fLocalBuf.buf_len - fLocalBuf.buf_read_counter;
        // ȷ��Ҫ��ȡ���ֽ���
        readLen = fLocalBuf.buf_len - fLocalBuf.buf_read_counter;

        // ��ȡ��Щ�ֽ�
        memcpy(fTo, (fLocalBuf.buf_data + fLocalBuf.buf_read_counter), readLen);
        //fMaxSize += fLocalBuf.buf_len - fLocalBuf.buf_read_counter;

        // �Ѿ���ȡ�ֽ���ͳ��ֵ
        fLocalBuf.buf_read_counter += readLen;

        // ��ǰFrame Size
        fFrameSize += readLen;
    }
                
    // ����Ѿ�����һ��buffer
    if(fLocalBuf.buf_read_counter == fLocalBuf.buf_len)
    {
        while(fPTsBuf->buf_writing !=0 )
        {
            printf("=== zyl === waiting for buf_writing\n");
        };
        #if 0
        for(i = 0; i < 188; i++)
        {
            printf("%02x, ", fPTsBuf->buf_data[i]);
            if ((i+1)%16 == 0)
                printf("\n");
        }
        printf("\n");
        #endif
        memcpy(fLocalBuf.buf_data, fPTsBuf->buf_data, fPTsBuf->buf_len);
        fLocalBuf.buf_read_counter = 0;
        fLocalBuf.buf_len = fPTsBuf->buf_len;
    }

    // ����Ѿ���ȡ���ֽ�������
    if(fFrameSize < fPreferredFrameSize)
    {
        // ����Ҫ��ȡ��Щ�ֽڵ�����
        readLen = fPreferredFrameSize - fFrameSize;

        // ��ȡ��Щ�ֽڣ���Ȼ����ʼ��ַ��Ҫ�ı�һ��
        memcpy(fTo+fFrameSize, (fLocalBuf.buf_data + fLocalBuf.buf_read_counter), readLen);
        
        // �Ѿ���ȡ�ֽ���ͳ��ֵ
        fLocalBuf.buf_read_counter += readLen;
        
        // ��ǰFrame Size
        fFrameSize += readLen;
    }

    // ���������buffer��һ���ֽڲ���0x47
    
    while(TRANSPORT_SYNC_BYTE != fTo[syncBytePosition])
    {
        syncBytePosition++;
    }
    if(0 != syncBytePosition)
    {
        printf("=== zyl === syncBytePosition !=0\n");
        memmove(fTo, &fTo[syncBytePosition], fFrameSize - syncBytePosition);
        fFrameSize -= syncBytePosition;
        
        // ����Ѿ���ȡ���ֽ�������
        if(fFrameSize < fPreferredFrameSize)
        {
            // ����Ҫ��ȡ��Щ�ֽڵ�����
            readLen = fPreferredFrameSize - fFrameSize;

            // ��ȡ��Щ�ֽڣ���Ȼ����ʼ��ַ��Ҫ�ı�һ��
            memcpy(fTo+fFrameSize, (fLocalBuf.buf_data + fLocalBuf.buf_read_counter), readLen);
            
            // �Ѿ���ȡ�ֽ���ͳ��ֵ
            fLocalBuf.buf_read_counter += readLen;
            
            // ��ǰFrame Size
            fFrameSize += readLen;
        }
    }
    

    //printf("=== zyl === ,fLocalBuf.buf_read_counter = %d, fLocalBuf.buf_len = %d\n",
    //            fLocalBuf.buf_read_counter, fLocalBuf.buf_len);

    if (fFrameSize == 0) 
    {
        handleClosure();
        return;
    }
    //fNumBytesToStream -= fFrameSize;

    // Set the 'presentation time':
    if (fPlayTimePerFrame > 0 && fPreferredFrameSize > 0) 
    {
        if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0) 
        {
            // This is the first frame, so use the current time:
            gettimeofday(&fPresentationTime, NULL);
        } 
        else
        {
            // Increment by the play time of the previous data:
            unsigned uSeconds	= fPresentationTime.tv_usec + fLastPlayTime;
            fPresentationTime.tv_sec += uSeconds/1000000;
            fPresentationTime.tv_usec = uSeconds%1000000;
        }

        // Remember the play time of this data:
        fLastPlayTime = (fPlayTimePerFrame*fFrameSize)/fPreferredFrameSize;
        fDurationInMicroseconds = fLastPlayTime;
    } 
    else 
    {
        // We don't know a specific play time duration for this data,
        // so just record the current time as being the 'presentation time':
        gettimeofday(&fPresentationTime, NULL);
    }

    // Inform the reader that he has data:
    // Because the file read was done from the event loop, we can call the
    // 'after getting' function directly, without risk of infinite recursion:
    FramedSource::afterGetting(this);
#endif	
}


