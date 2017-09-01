//
// Created by Administrator on 2017/8/21 0021.
//


#include "RecordFilter.h"
#include "../encode/jx_ffmpeg_config.h"
#include "../encode/jx_log.h"

void RecordFilter::create(const char *vertex, const char *texture) {
    BaseFilter::create(vertex, texture);
}

void RecordFilter::onDrawArraysPre() {
}

void RecordFilter::onDrawArraysAfter() {
    if (isEncode) {
        bindPixelBuffer();
    }
}

void RecordFilter::onCreated() {

}

void RecordFilter::initFramebuffer(int width, int height) {
    BaseFilter::initFramebuffer(width, height);
}

void RecordFilter::destroyFrameBuffer() {
    BaseFilter::destroyFrameBuffer();
}

int RecordFilter::drawToFrameBuffer(GLenum target, GLuint texture) {
    if (isEncode) {
        return BaseFilter::drawToFrameBuffer(target, texture, VERTICES_RENDER, TEXTURE_180_RENDER);
    }
    return 0;
}

void RecordFilter::drawFrame(GLenum target, GLuint texture) {
    BaseFilter::drawFrame(target, texture);
}

bool RecordFilter::isProgramAvailable() {
    return BaseFilter::isProgramAvailable();
}

void RecordFilter::initPixelBuffer(int width, int height) {
    if (*mPixelBuffers != NULL && (frameWidth != width || frameHeight != height)) {
        destroyPixelBuffer();
    }
    if (*mPixelBuffers != NULL) {
        return;
    }

    int align = 4;//128字节对齐
    //这个字节对齐很重要，如果没有字节对齐，效率上不会有任何的提升
//    mRowStride = (width*4 + (align - 1)) & ~(align - 1);
    mRowStride = (width*4 + (align - 1)) & ~(align - 1);
    mPboSize = mRowStride * height;

    glGenBuffers(2, mPixelBuffers);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, mPixelBuffers[0]);
    glBufferData(GL_PIXEL_PACK_BUFFER, mPboSize, NULL, GL_STATIC_READ);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, mPixelBuffers[1]);
    glBufferData(GL_PIXEL_PACK_BUFFER, mPboSize, NULL, GL_STATIC_READ);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

void RecordFilter::destroyPixelBuffer() {
    if (*mPixelBuffers != NULL) {
        glDeleteBuffers(2, mPixelBuffers);
        *mPixelBuffers = NULL;
    }
}

void RecordFilter::bindPixelBuffer() {
    if (*mPixelBuffers == NULL) {
        return;
    }
    glBindBuffer(GL_PIXEL_PACK_BUFFER, mPixelBuffers[mPboIndex]);
    glReadPixels(0, 0, mRowStride / 4, frameHeight, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    if (mInitRecord) {//第一帧没有数据跳出
        unbindPixelBuffer();
        mInitRecord = false;
        return;
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, mPixelBuffers[mPboNewIndex]);

    //glMapBufferRange会等待DMA传输完成，所以需要交替使用pbo
    uint8_t * buffer = (uint8_t *) glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, mPboSize, GL_MAP_READ_BIT);
    LOGI(JNI_DEBUG, "start copy");
    size_t in_y_size = (size_t) (frameWidth * frameHeight * 4);
    uint8_t *new_buf = (uint8_t *) malloc(in_y_size);
    memcpy(new_buf, buffer, in_y_size);
    LOGI(JNI_DEBUG, "end copy");
    encodeFrame2H264(new_buf);
    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    unbindPixelBuffer();

}

//解绑pbo
void RecordFilter::unbindPixelBuffer() {
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    mPboIndex = (mPboIndex + 1) % 2;
    mPboNewIndex = (mPboNewIndex + 1) % 2;
}

void RecordFilter::end_notify() {
    try {
        env->CallStaticVoidMethod(java_class, pID, mPboSize);
    }
    catch (exception e) {
    }
}

void RecordFilter::onPrepareToRender() {
    glViewport(0, 0, frameWidth, frameHeight);
}

void RecordFilter::setFrameSize(int width, int height) {
    BaseFilter::setFrameSize(width, height);
}

void RecordFilter::startRecord() {
    isEncode = true;
    mInitRecord = true;
    mPboIndex = 0;
    mPboNewIndex = 1;
    pthread_t thread;
//    pthread_create(&thread, NULL, RecordFilter::encodeFrame, this);
}

void RecordFilter::endRecord() {
    isEncode = false;
}

void RecordFilter::encodeFrame(void *obj) {

}









