# STM32-DSP性能软件测试说明
如下展示通过MDK软件仿真进行STM32对应型号，执行DSP函数所需运行时间。

代码: https://github.com/SillyPigeon/stm32_dsp_flow_test_proj

**注意**：

**1、由于MDK软件仿真功能有限，只支持stm32f1x系列的完整芯片仿真**

​      **对于需求的型号，MDK只支持使用下述内核模型仿真 [µVision User's Guide (arm.com)](https://developer.arm.com/documentation/101407/0538/Simulation)**

- **Arm Cortex-M0 / M0+ / M3 / M4 / M7* / M23* / M33* / M35P* / M55* / M85***
- **Arm SecurCore 000 / 300**
- **ARM7** **and ARM9**

**2、对于stm32的f7与h7模型，由于Arm Cortex-M7* 只支持在AVH(arm虚拟机)中执行，所以对于型号是通过MDK中contex-M4的模型，执行对应指令模拟运行**

**3、对于stm32h745 双核芯片，MDK无法软件仿真模拟多核执行，仅测试其内部M7核的编码执行效率**

**4、对于stm32mp1 的contex-A7核，MDK无法进行软件仿真**



