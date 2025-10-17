package com.lynx.tasm.behavior.ui.list.container;

/**
 * Created by hujing.1 on 2025/10/16
 *
 * @author hujing.1@bytedance.com
 */
public class ListContainerProxy {
  
  private UIListContainer mUIList;
  private long nativeEnginePtr;
  private long nativeContainerPtr;
  
  public ListContainerProxy(UIListContainer uiList,long ptr) {
    mUIList = uiList;
    nativeEnginePtr = ptr;
    nativeContainerPtr = nativeCreate(nativeEnginePtr);
  }
  

  // create 'list_container_proxy' C++ object
  private native long nativeCreate(long enginePtr);

  private native void nativeScrollByListContainer(
    long ptr, long lifecycle, int sign, float x, float y, float originalX, float originalY);

  private native void nativeScrollToPosition(
    long ptr, long lifecycle, int sign, int position, float offset, int align, boolean smooth);

  private native void nativeScrollStopped(long ptr, long lifecycle, int sign);
  
  // release 'list_container_proxy' C++ object
  private native void nativeDestroy(long containerPtr);
  

  private ListContainerProxy() {
  }
  
  public void scrollByListContainer(
    long lifecycle, int sign, float x, float y, float originalX, float originalY) {
    nativeScrollByListContainer(nativeContainerPtr, lifecycle, sign, x, y, originalX, originalY);
  }

  public void scrollToPosition(
    long lifecycle, int sign, int position, float offset, int align, boolean smooth) {
    nativeScrollToPosition(nativeContainerPtr, lifecycle, sign, position, offset, align, smooth);     
    }
  
  public void scrollStopped(long lifecycle, int sign) {
    nativeScrollStopped(nativeContainerPtr, lifecycle, sign);
  }
  
  private void destroy() {
    nativeDestroy(nativeContainerPtr);
  }
  
}
