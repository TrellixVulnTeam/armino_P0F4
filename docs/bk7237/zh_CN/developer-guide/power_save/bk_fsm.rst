系统状态机说明
=============================================

:link_to_translation:`en:[English]`


.. image:: ../../../_static/pm_pcm.png



系统支持不同的睡眠模式:
 - active
 - normal sleep
 - low voltage
 - deepsleep
 - shut down



active(正常工作)
--------------------------------------------
 - CPU处于工作状态和WIFI,BT可以正常收发数据。


normal sleep(普通睡眠)
++++++++++++++++++++++++++++++++++++++++++++
 - RTOS没有任务需要处理时，系统进入IDLE任务中，在IDLE任务中，CPU会进入WFI睡眠；当有任何中断到来时，都能让系统退出WFI状态，进入正常工作。


low voltage(低压睡眠)
++++++++++++++++++++++++++++++++++++++++++
 - 低压睡眠是一种相对比较节省功耗的睡眠模式。在该模式下系统只有32K时钟，此时只有部分硬件模块在工作，除了AON模块其他硬件模块在低压下暂停运行。

当唤醒信号触发后，系统退出低压状态，电压升级到正常电压。
 - 处于低压状态下，以下唤醒源(GPIO,RTC,Touch,WIFI,BT)可以让系统退出低压。注意：WIFI,BT进入是根据WIFI，BT的业务场景来投票进入，比较特殊。
 
 - 该模式下32K的时钟源可以根据自己业务的使用场景选择，参考时钟说明。
 
 - 为了达到最优功耗，不需要的模块，进入低压前请关闭，退出低压后，可以再开启。

deep sleep(深度睡眠)
++++++++++++++++++++++++++++++++++++++++++
 - 深度睡眠是一种相对最节省功耗的睡眠模式。在该模式下系统只有32K时钟，此时只有部分硬件模块在工作，除了AON模块其他硬件模块都已经下电。

当唤醒信号触发后，系统深度睡眠状态，系统复位。
 - 处于深度睡眠状态下，以下唤醒源(GPIO,RTC,Touch)可以让系统退出深度睡眠。
 
 - 该模式下32K的时钟源默认是使用ROSC的32K。


shut down(关机)
--------------------------------------------
 - 整个系统都已经下电



状态机切换说明
=============================================
 - 低压睡眠和深度睡眠是系统性睡眠，如果系统进入了低压低压睡眠和深度睡眠，个别模块还在工作，该模块的异常退出对系统从低压和深度睡眠中唤醒后，可能不能正常工作，
 
 为了从机制上避免该问题的发生，系统是通过投票机制进入低压睡眠和深度睡眠。

 - 低压睡眠：当前进入低压睡眠一共设置了3张票：BT,WIFI,APP。BT,WIFI的票是BT,WIFI业务中根据自己的工作流程投的票，外界无需关注。APP的票应用程序使用者可以根据自己的使用场景来投票。
 
 另外当前系统支持设置32张票，我们会根据场景需求设置。对于SDK之外不需要关注太多，只需要关注APP这一张票。

 - 深度睡眠：当前进入深度睡眠一共设置了2张票：BT,WIFI。由于深度睡眠唤醒后，系统会重启，因此BT,WIFI没有特别需要保存的内容，因此强制投上BT,WIFI的票。
 
 - 系统中RTOS没有任务处理时，自动进入IDLE任务，进行WFI。当满足了低压条件（满足了进入低压的票时）进入进入低压状态。
 