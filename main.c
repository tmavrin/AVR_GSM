#define F_CPU 7372800UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lcd.h"
#include <avr/interrupt.h>


#define FOSC 7372800UL// Clock Speed
#define BAUD 9600
#define MYUBRR FOSC/16/BAUD-1 // Ne radi -> ovo iznosi 47 -> 51 radi

unsigned int messageIndex= 1;

void USART_Init( unsigned int ubrr){
	/* Set baud rate */
	UBRRH = (unsigned char)(ubrr>>8);
	UBRRL = (unsigned char)ubrr;
	/* Enable receiver and transmitter */
	UCSRB = (1 << RXEN) | (1 << TXEN);
	/* Set frame format: 8data, 2stop bit */
	UCSRC = (1 << URSEL) | (1 << USBS) | (3 << UCSZ0);
}

unsigned char USART_Receive( void ){

	/* Wait for data to be received */
	while ( !(UCSRA & (1<<RXC)) )
	;
	/* Get and return received data from buffer */
	return UDR;
}

void USART_Receive_Line(char data[] ){
	unsigned int i = 0;
	while(data[i-1] != 13){
		data[i] = USART_Receive();
		i++;
	}
	data[i] = USART_Receive();
	data[i-1] = '\0';
}

void USART_Transmit( unsigned char data ){
	
	/* Wait for empty transmit buffer */
	while ( !( UCSRA & (1<<UDRE)) )
	;
	/* Put data into buffer, sends the data */
	UDR = data;
	
}

void USART_Transmits(char data[] ) {
	int i;

	for(i = 0; i < strlen(data); i++) {
		USART_Transmit(data[i]);
		_delay_ms(25);
	}
}


void USART_Wait_For(char data[]){
	char line[64];

	int compare = 1;
	while(compare != 0){
		USART_Receive_Line(line);
		compare = strncmp(data, line, strlen(data));
		lcd_clrscr();
		lcd_puts(line);
	}
}

void USART_Retry_Until(char command[], char message[]){
	char line[64];
	USART_Transmits(command);
	
	int compare = 1;
	while(compare != 0){
		USART_Receive_Line(line);
		compare = strncmp(message, line, strlen(message));
		
		if(compare == 0){
			break;
			} else {
			_delay_ms(1000);
			USART_Transmits(command);
		}
	}
}

void GSM_Wait_For_Boot(){
	char sind11[] = "+SIND: 11";
	_delay_ms(1000);
	USART_Wait_For(sind11); // waiting for SIND: 11 -> connected to network status
	lcd_clrscr();
	lcd_puts("Success!\nConnected!\n");
}

void GSM_Send_SMS(char *sms, char *number){
	lcd_puts("SENDING SMS");
	lcd_clrscr();
	_delay_ms(3000);

	// AT+CMGS="xxxxx" // phone number
	// >
	// poruka0x1A\r
	// OK
	USART_Transmits("AT+CMGS=\"");
	USART_Transmits(number);
	USART_Transmits("\"\r");
	
	_delay_ms(5000);
	
	USART_Transmits(sms);
	USART_Transmits("\x1A\r");
	USART_Wait_For("OK");
	lcd_clrscr();
}

void GSM_Read_Msg(unsigned int index){
	
	_delay_ms(1000);
	
	lcd_clrscr();
	
	char command[] = "AT+CMGR=0\r\0";
	command[8]= 48+index;
	
	USART_Transmits(command);
	
	char message1[128];
	char message2[128];

	
	USART_Receive_Line(message1); // get first line in message ignore that
	USART_Receive_Line(message2); // get the actual message
	
	UDR = 0x00; //clear buffer (might get "OK" or nothing, clear eather way)
	
	lcd_clrscr();
	lcd_puts(message1);
	_delay_ms(200);
	lcd_clrscr();
	lcd_puts(message2);
}

void debounce() {
	_delay_ms(2000);
	GIFR = _BV(INTF0) | _BV(INTF1);
}

// Prikazujemo prijasnju poruku (po indexu)
ISR(INT0_vect) {
	if(messageIndex > 1){
		messageIndex--;
	}
	lcd_clrscr();
	GSM_Read_Msg(messageIndex);
	debounce();
}

// Prikazujemo sljedecu poruku (po indexu)
ISR(INT1_vect) {
	if(messageIndex < 9){
		messageIndex++;	
	}
	lcd_clrscr();
	GSM_Read_Msg(messageIndex);
	debounce();
}


void Init_Prog(){
	DDRD = _BV(4);
	USART_Init ( 51 );
	lcd_init(LCD_DISP_ON);
	lcd_clrscr();
	
	//interrupts
	MCUCR = _BV(ISC01) | _BV(ISC11);
	GICR = _BV(INT0) | _BV(INT1);
	sei();
}


int main( void ){

	Init_Prog();
	
	_delay_ms(2000);
	
	lcd_puts("Booting...");
	
	//Cekamo "+SIND: 11" Koji znaci da se modul spojio na toranj
	GSM_Wait_For_Boot();

	//_delay_ms(5000);
	lcd_clrscr();
	
	// Saljemo poruku, koristimo komande:
	// AT+CMGF da bi postavili text mode
	// AT+CMGS da bu poslali poruku
	
	USART_Retry_Until("AT+CMGF=1\r", "OK");
	
	// Simuliramo zahtjev s slanjem poruke "Zagreb" za
	// vremensku prognozu u Zagrebu
	lcd_puts("Sending message");
	_delay_ms(3000);
	GSM_Send_SMS("Zagreb", "xxxxxxxxxx"); // insert phone number here
	
	//lcd_clrscr();
	
	// Cekamo 30s da se vrati poruka
	lcd_puts("Waiting 30s for\nresponse");
	_delay_ms(30000);
	lcd_clrscr();
	
	// Citamo poruku na indexu
	GSM_Read_Msg(7);
	// read message at index,
	//"ALL" method is unreliable
	//index should be 1-9, last index is last message

	while(1){
		_delay_ms(500);
	}
	
	return 0;
}