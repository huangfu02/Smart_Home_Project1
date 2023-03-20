#define F_CPU 8000000UL			/* Define CPU Frequency e.g. here 8MHz */
#include <avr/io.h>			/* Include AVR std. library file */
#include <util/delay.h>			/* Include Delay header file */
#include <stdio.h>
#include <string.h>
#define KEY_PRT 	PORTA
#define KEY_DDR		DDRA
#define KEY_PIN		PINA
#define LCD_Dir  DDRB			/* Define LCD data port direction */
#define LCD_Port PORTB			/* Define LCD data port */
#define RS PB0				/* Define Register Select pin */
#define EN PB1 				/* Define Enable signal pin */

void LCD_Command( unsigned char cmnd )
{
	LCD_Port = (LCD_Port & 0x0F) | (cmnd & 0xF0); /* sending upper nibble */
	LCD_Port &= ~ (1<<RS);		/* RS=0, command reg. */
	LCD_Port |= (1<<EN);		/* Enable pulse */
	_delay_us(1);
	LCD_Port &= ~ (1<<EN);

	_delay_us(200);

	LCD_Port = (LCD_Port & 0x0F) | (cmnd << 4);  /* sending lower nibble */
	LCD_Port |= (1<<EN);
	_delay_us(1);
	LCD_Port &= ~ (1<<EN);
	_delay_ms(2);
}

void LCD_Char( unsigned char data )
{
	LCD_Port = (LCD_Port & 0x0F) | (data & 0xF0); /* sending upper nibble */
	LCD_Port |= (1<<RS);		/* RS=1, data reg. */
	LCD_Port|= (1<<EN);
	_delay_us(1);
	LCD_Port &= ~ (1<<EN);

	_delay_us(200);

	LCD_Port = (LCD_Port & 0x0F) | (data << 4); /* sending lower nibble */
	LCD_Port |= (1<<EN);
	_delay_us(1);
	LCD_Port &= ~ (1<<EN);
	_delay_ms(2);
}

void LCD_Init (void)			/* LCD Initialize function */
{
	LCD_Dir = 0xFF;			/* Make LCD port direction as o/p */
	_delay_ms(20);			/* LCD Power ON delay always >15ms */
	
	LCD_Command(0x02);		/* send for 4 bit initialization of LCD  */
	LCD_Command(0x28);              /* 2 line, 5*7 matrix in 4-bit mode */
	LCD_Command(0x0c);              /* Display on cursor off*/
	LCD_Command(0x06);              /* Increment cursor (shift cursor to right)*/
	LCD_Command(0x01);              /* Clear display screen*/
	_delay_ms(2);
}

void LCD_String (char *str)		/* Send string to LCD function */
{
	int i;
	for(i=0;str[i]!=0;i++)		/* Send each char of string till the NULL */
	{
		LCD_Char (str[i]);
	}
}

void LCD_String_xy (char row, char pos, char *str)	/* Send string to LCD with xy position */
{
	if (row == 0 && pos<16)
	LCD_Command((pos & 0x0F)|0x80);	/* Command of first row and required position<16 */
	else if (row == 1 && pos<16)
	LCD_Command((pos & 0x0F)|0xC0);	/* Command of first row and required position<16 */
	LCD_String(str);		/* Call LCD string function */
}

void LCD_Clear()
{
	LCD_Command (0x01);		/* Clear display */
	_delay_ms(2);
	LCD_Command (0x80);		/* Cursor at home position */
}

unsigned char keypad[4][4] = {	{'7','8','9','/'},
								{'4','5','6','*'},
								{'1','2','3','-'},
								{' ','0','=','+'}};

unsigned char colloc, rowloc;

char keyfind()
{
	while(1)
	{
		KEY_DDR = 0xF0;           	/* set port direction as input-output */
		KEY_PRT = 0xFF;

		do
		{
			KEY_PRT &= 0x0F;      		/* mask PORT for column read only */
			asm("NOP");
			colloc = (KEY_PIN & 0x0F); 	/* read status of column */
		}while(colloc != 0x0F);
		
		do
		{
			do
			{
				_delay_ms(20);             /* 20ms key debounce time */
				colloc = (KEY_PIN & 0x0F); /* read status of column */
				}while(colloc == 0x0F);        /* check for any key press */
				
				_delay_ms (40);	            /* 20 ms key debounce time */
				colloc = (KEY_PIN & 0x0F);
			}while(colloc == 0x0F);

			/* now check for rows */
			KEY_PRT = 0xEF;            /* check for pressed key in 1st row */
			asm("NOP");
			colloc = (KEY_PIN & 0x0F);
			if(colloc != 0x0F)
			{
				rowloc = 0;
				break;
			}

			KEY_PRT = 0xDF;		/* check for pressed key in 2nd row */
			asm("NOP");
			colloc = (KEY_PIN & 0x0F);
			if(colloc != 0x0F)
			{
				rowloc = 1;
				break;
			}
			
			KEY_PRT = 0xBF;		/* check for pressed key in 3rd row */
			asm("NOP");
			colloc = (KEY_PIN & 0x0F);
			if(colloc != 0x0F)
			{
				rowloc = 2;
				break;
			}

			KEY_PRT = 0x7F;		/* check for pressed key in 4th row */
			asm("NOP");
			colloc = (KEY_PIN & 0x0F);
			if(colloc != 0x0F)
			{
				rowloc = 3;
				break;
			}
		}

		if(colloc == 0x0E)
		return(keypad[rowloc][0]);
		else if(colloc == 0x0D)
		return(keypad[rowloc][1]);
		else if(colloc == 0x0B)
		return(keypad[rowloc][2]);
		else
		return(keypad[rowloc][3]);
	}
	
#define PASSWORD_LENGTH 4

char password[PASSWORD_LENGTH+1] = "1234"; // M?t kh?u m?c ??nh

void enter_password_mode(void);
void change_password_mode(void);
void display_menu(void);
int verify_password(char* entered_password);

// Hi?n th? menu
void display_menu(void)
{
	LCD_String_xy(0,0,"Enter pass: +");
	LCD_String_xy(1,0,"Change pass: -");
}

// Ch? ?? nh?p m?t kh?u
void enter_password_mode(void)
{
	char entered_password[PASSWORD_LENGTH+1];
	int i;

	LCD_Clear();
	LCD_String_xy(0,0,"Enter pass:");
	for(i = 0; i < PASSWORD_LENGTH; i++)
	{
		entered_password[i] = keyfind();
		LCD_Char('*');
	}
	entered_password[PASSWORD_LENGTH] = '\0';

	if(verify_password(entered_password)) // N?u m?t kh?u ?úng
	{
		LCD_Clear();
		LCD_String_xy(0,0,"Door Opened");
		_delay_ms(1000);
		LCD_Clear();
		display_menu();
		// M? c?a t?i ?ây
	}
	else // N?u m?t kh?u sai
	{
		LCD_Clear();
		LCD_String_xy(0,0,"Incorrect pass");
		_delay_ms(1000);
		LCD_Clear();
		display_menu();
	}
}

// Ch? ?? ??i m?t kh?u
void change_password_mode(void)
{
	char entered_password[PASSWORD_LENGTH+1];
	int i;

	LCD_Clear();
	LCD_String_xy(0,0,"Old pass:");
	for(i = 0; i < PASSWORD_LENGTH; i++)
	{
		entered_password[i] = keyfind();
		LCD_Char('*');
	}
	entered_password[PASSWORD_LENGTH] = '\0';

	if(verify_password(entered_password)) // N?u m?t kh?u ?úng
	{
		LCD_Clear();
		LCD_String_xy(0,0,"New pass:");
		for(i = 0; i < PASSWORD_LENGTH; i++)
		{
			password[i] = keyfind();
			LCD_Char('*');
		}
		password[PASSWORD_LENGTH] = '\0';
		LCD_Clear();
		LCD_String_xy(0,0,"Password changed");
		_delay_ms(1000);
		LCD_Clear();
		display_menu();
	}
	else // N?u m?t kh?u sai
	{
		LCD_Clear();
		LCD_String_xy(0,0,"Incorrect pass");
		_delay_ms(1000);
		LCD_Clear();
		display_menu();
	}
}

// Xác minh m?t kh?u
int verify_password(char* entered_password)
{
	return (strcmp(entered_password, password) == 0);
}

int main(void)
{
	LCD_Init();
	display_menu();
	while(1)
	{
		char key = keyfind();
		if(key == '+') // Ch? ?? nh?p m?t kh?u
		{
			enter_password_mode();
		}
		else if(key == '-') // Ch? ?? ??i m?t kh?u
		{
			change_password_mode();
		}
	}
}