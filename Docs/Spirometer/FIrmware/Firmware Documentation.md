
# float adc_to_slpm(uint32_t adc_value)

This function converts raw adc values from the sensor into slpm.

From the [[587d773494b6f.pdf]] we can get the nominal output voltage v.s. flow rate:

| Flow Rate (SLPM) | Nominal Voltage (Vdc) |
| ---------------- | --------------------- |
| 0                | 0.5                   |
| 20               | 1.3                   |
| 40               | 2.1                   |
| 60               | 2.9                   |
| 80               | 3.7                   |
| 100              | 4.5                   |
| 110              | 4.9                   |
| 120              | 4.9                   |
This is for 5v adc, but I have added a voltage divider and RC circuit for decoupling since the stm32 adc works on 3.3v. 

As per the datasheet, 
>The FS1015CL provides an analog output of 0.5 ~ 4.5 Vdc corresponding with 0 ~ full scale flow rate.

The 5v signal is converted into 3.3v signal with the help of this signal.

![[Pasted image 20260113190437.png]]

$$
\text{Divider Ratio} = \frac{3.3}{4.5} = 0.733
$$
The table becomes:


| Flow Rate (SLPM) | ADC Voltage @ 3.3 V logic (V) |
| ---------------- | ----------------------------- |
| 0                | 0.37                          |
| 30               | 0.95                          |
| 60               | 1.54                          |
| 90               | 2.13                          |
| 120              | 2.71                          |
| **150**          | **3.30**                      |
| **165 (110%)**   | **3.59**                      |
| **>165**         | **3.59**                      |

I converted this linear relationship into an equation with the help of slope formula

$$
\text{Slope} = \frac{y_2 - y_1}{x_2 - x_1}
$$
$$
\text{Slope} = \frac{150 - 0}{3.30 - 0.37}
$$
$$
\text{Slope} = \frac{150}{2.93} \approx 51.19 \;\text{SLPM/V}
$$
So flow becomes:
$$
\text{Flow} = mV + c
$$
where

- m = slope (SLPM/V)
- c = intercept (SLPM)

$$
\boxed{
\text{Flow (SLPM)} = 51.19\,V - 18.94
}
$$

$$
\boxed{
\text{Flow} =
51.19 \left( \frac{\text{ADC}}{4095} \times 3.3 \right) - 18.94
}
$$
