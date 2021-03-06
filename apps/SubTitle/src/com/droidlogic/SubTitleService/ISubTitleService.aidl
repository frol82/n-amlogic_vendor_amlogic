package com.droidlogic.SubTitleService;

interface ISubTitleService
{
    void open(String path);
    void openIdx(int idx);
    void close();
    int getSubTotal();
    int getInnerSubTotal();
    int getExternalSubTotal();
    void nextSub();
    void preSub();
    //void startInSubThread();
    //void stopInSubThread();
    void showSub(int position);
    void option();
    int getSubType();
    String getSubTypeStr();
    int getSubTypeDetial();
    void setTextColor(int color);
    void setTextSize(int size);
    void setGravity(int gravity);
    void setTextStyle(int style);
    void setPosHeight(int height);
    void setImgSubRatio(float ratioW, float ratioH, int maxW, int maxH);
    void clear();
    void resetForSeek();
    void hide();
    void display();
    String getCurName();
    String getSubName(int idx);
    String getSubLanguage(int idx);
    boolean load(String path);
    void setSurfaceViewParam(int x, int y, int w, int h);
    void setIOType(int type);
    String getPcrscr();
}