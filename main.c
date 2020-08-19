#define F_CPU 7372800UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#define FOSC 7372800UL// Clock Speed
#define BAUD 9600
#define MYUBRR FOSC/16/BAUD-1 // Ne radi -> ovo iznosi 47 -> 51 radi

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
	while(data[i-1] != '\r'){
		data[i] = USART_Receive();
		i++;
	}
	data[i] = '\0';
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


void USART_Wait_For( char data[], unsigned int chars){
	char line[32];
	
	int compare = 1;
	while(compare != 0){
		USART_Receive_Line(line);
		compare = strncmp(data, line, chars);	
	}
}


int main( void ){
	USART_Init ( 51 );

	_delay_ms(5000);
	USART_Transmits("Hello There!\r\r");
	
	_delay_ms(2000);
	USART_Transmits("General Kenobi!\r");

	while(1){
		char str[] = "+SIND: 11";
		USART_Wait_For(str, strlen(str));
		
		USART_Transmits("GOT SIND 11");
		_delay_ms(10000);
	}
	return 0;
}