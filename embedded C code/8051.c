#include<reg52.h>
#include<math.h>
#include<stdio.h>  
#include <string.h>

unsigned char uart_done;
sfr ldata=0xa0;//port2
sbit rd=P3^5;
sbit wr=P3^6;
sbit intr=P3^7;
sfr adc=0x90;//port1   
sbit rs = P3^2;  //RS pin connected to pin 2 of port 3
sbit X = P3^3;  //Relay Output
sbit e =  P3^4;  //E pin connected to pin 4 of port 3


//LCD
void msdelay(unsigned int delay)
{   
    unsigned int i;
    TMOD = 0x01;  // Timer 0 Mode 1 (16-bit timer)
    
    for (i = 0; i < delay; i++) {
        TH0 = 0xFC;    // Load high byte for 1ms delay
        TL0 = 0x18;    // Load low byte
        TR0 = 1;       // Start Timer0
        while (TF0 == 0);  // Wait until overflow
        TR0 = 0;       // Stop Timer0
        TF0 = 0;       // Clear overflow flag
    }
}
void lcd_cmd(unsigned char a)
{
  rs=0;//cmd 
	ldata=a;
	e=1;
	msdelay(5);
	e=0;
	msdelay(5);
}
void lcd_data(unsigned char b)
{
  rs=1; //dat
	ldata=b;
	e=1;
	msdelay(100);
	e=0;
	msdelay(100);
}
void lcd_str(unsigned char *str)
{
while(*str)
{
	lcd_data(*str++);
}
}

 void lcd_init()    //Function to prepare the LCD  and get it ready
{   
    lcd_cmd(0x38);  // for usin 2 lines and 5X7 matrix of LCD
    msdelay(10);
    lcd_cmd(0x0F);  // turn display ON, cursor blinking
    msdelay(10);
    lcd_cmd(0x01);  //clear screen
    msdelay(10);
    lcd_cmd(0x80);  // bring cursor to position 1 of line 1
    msdelay(10);
}	
void display_current(unsigned char adc_value) {
    
    float voltage, current;
	  char buffer[10];
    //unsigned int int_part, decimal_part;

    voltage = (adc_value * 5.0) / 255.0; // Convert to voltage
    current = fabs((voltage - 2.5) / 0.185);   // Convert to current (ACS712-5A)
	  
	
	  lcd_cmd(0x80); // Move to first line
    lcd_str("Current:");

    sprintf(buffer, "%.2f A", current); // Format output with 2 decimal places
    lcd_str(buffer);

   /* int_part = (unsigned int)current;
    decimal_part = (unsigned int)((current - int_part) * 100);

    lcd_cmd(0x80); // Move to first line
    lcd_str("Current:");
    lcd_data(int_part + '0'); // Display integer part
    lcd_data('.');
    lcd_data((decimal_part / 10) + '0');
    lcd_data((decimal_part % 10) + '0');
    lcd_str(" A");
	  
	  if (~(0<= adc_value<=255)) {  // If no current detected
            lcd_str("Current:0A");
		}*/
	  if (current>3)
		{ X=1;
			lcd_cmd(0x01);
			lcd_cmd(0x80);
			lcd_str("OVERLOAD");
			lcd_cmd(0xc0);
			lcd_str("CIRCUIT BREAKED");
		}
			
}
//UART

//volatile unsigned char uart_done = 0;  // Define uart_done globally

void uart_init(unsigned int baud)
{
    TMOD = 0x20;  // Timer 1 mode 2 (auto-reload)
    SCON = 0x50;  // UART mode 1, 8-bit data

    if (baud == 9600)
    {
        TL1 = 0xFD;
        TH1 = 0xFD;
    }
    else if (baud == 4800)
    { 
        TL1 = 0xFA;
        TH1 = 0xFA;
    }
    else if (baud == 2400)
    {
        TL1 = 0xF4;
        TH1 = 0xF4;
    }
    else if (baud == 1200)
    {
        TL1 = 0xE8;
        TH1 = 0xE8;
    }

    TR1 = 1; // Start Timer 1
}
 

void uart_write(unsigned char value)
{
	SBUF = value;
	while(!TI);
	TI = 0;
}

char uart_read()
{
	while(!RI);
	RI = 0;
	return (SBUF);
}
void uart_write_text(char *str)
{
	unsigned char i = 0;
	while(str[i])
		uart_write(str[i++]);
}

void uart_read_text(char *buff, unsigned int len) 
{
    unsigned int i;
    unsigned int timeout = 50000;
    uart_done=0;
	  while (!RI) {  // Wait for UARTdata
            timeout--;
            if (timeout == 0) {
                buff[i] = '\0'; // Stop if nodata
                return;
            }
					}
     for (i = 0; i < len; i++) {
         buff[i] = uart_read();
        }  
    buff[len] = '\0';  // Null-terminate the string
		uart_done=1;
}

void main()
{
	char recieved_data[5]; // 4 characters + null terminator		
	uart_init(9600);
	msdelay(10);
	X =0;
	lcd_init();
	lcd_str("   WELCOME TO   ");
	lcd_cmd(0xc0);
	lcd_str("  OUR PROJECT   ");
	msdelay(1000);
	lcd_cmd(0x01);
	lcd_cmd(0x80);
  lcd_str("CIRCUIT BREAKER");
	lcd_cmd(0xc0);
	lcd_str(" USING PASSWORD ");
	msdelay(1000);
	lcd_cmd(0x01);
	lcd_cmd(0x80);
	msdelay(1000);
	while(1)
  {
		uart_read_text(recieved_data, 4);
	  if (uart_done ==1) // Corrected string comparison
		{ lcd_cmd(0x01);
		  lcd_cmd(0x80);
			if (strcmp(recieved_data, "1234") == 0)
			{
				X =1; // Turn OFF relay
			  lcd_str("ACCESS GRANTED");
		  }
			else if (strcmp(recieved_data, "5678") == 0)
			{
				X =0; // Turn ON relay
			  lcd_str("ACCESS GRANTED");
		  }
			else
			{
			  lcd_str("ACCESS DENIED");
			}	
	  }
	rd=1;
	wr=0;
	msdelay(1000);
	wr=1;
  while(intr==1);//until conversion finished 
		msdelay(1000);
    rd=0;
		lcd_cmd(0x80);
	  display_current(adc);
    intr=1;//next conversion 

}
}
