#if defined(SHENQI_NRF24L01_INO)

#include "iface_nrf24l01.h"

const uint8_t PROGMEM SHENQI_Freq[] = {
			50,50,20,60,30,40,
			10,30,40,20,60,10,
			50,20,50,40,10,60,
			30,30,60,10,40,50,
			20,10,60,20,50,30,
			40,40,30,50,20,60,
			10,10,20,30,40,50,
			60,60,50,40,30,20,
			10,60,10,50,30,40,
			20,10,40,30,60,20 };

void SHENQI_init()
{
    NRF24L01_Initialize();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);		// Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);			// No Auto Acknowldgement on all data pipes
	NRF24L01_SetBitrate(NRF24L01_BR_1M);          // 1Mbps
    NRF24L01_SetPower();

    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);		// 5 bytes rx/tx address

	LT8910_Config(4, 8, _BV(LT8910_CRC_ON)|_BV(LT8910_PACKET_LENGTH_EN), 0xAA);
	LT8910_SetChannel(2);
	LT8910_SetAddress((uint8_t *)"\x9A\x9A\x9A\x9A",4);
	LT8910_SetTxRxMode(RX_EN);
}

void SHENQI_send_packet()
{
	packet[0]=0x00;
	if(packet_count==0)
	{
		uint8_t bind_addr[4];
		bind_addr[0]=0x9A;
		bind_addr[1]=0x9A;
		bind_addr[2]=rx_tx_addr[2];
		bind_addr[3]=rx_tx_addr[3];
		LT8910_SetAddress(bind_addr,4);
		LT8910_SetChannel(2);
		packet[1]=rx_tx_addr[1];
		packet[2]=rx_tx_addr[0];
		packet_period=2508;
	}
	else
	{
		LT8910_SetAddress(rx_tx_addr,4);
		packet[1]=255-convert_channel_8b(RUDDER);
		packet[2]=255-convert_channel_8b_scale(THROTTLE,0x60,0xA0);
		uint8_t freq=pgm_read_byte_near(&SHENQI_Freq[hopping_frequency_no])+(rx_tx_addr[1]&0x0F);
		LT8910_SetChannel(freq);
		hopping_frequency_no++;
		if(hopping_frequency_no==60)
			hopping_frequency_no=0;
		packet_period=1750;
	}
	// Send packet + 1 retransmit - not sure why but needed (not present on original TX...)
	LT8910_WritePayload(packet,3);
	while(NRF24L01_packet_ack()!=PKT_ACKED);
	LT8910_WritePayload(packet,3);
	
	packet_count++;
	if(packet_count==7)
	{
		packet_count=0;
		packet_period=3000;
	}
	// Set power
	NRF24L01_SetPower();
}

uint16_t SHENQI_callback()
{
	if(IS_BIND_DONE_on)
		SHENQI_send_packet();
	else
	{
		if( NRF24L01_ReadReg(NRF24L01_07_STATUS) & BV(NRF24L01_07_RX_DR))
		{
			if(LT8910_ReadPayload(packet, 3))
			{
				BIND_DONE;
				rx_tx_addr[3]=packet[1];
				rx_tx_addr[2]=packet[2];
				LT8910_SetTxRxMode(TX_EN);
				packet_period=14000;
			}
			NRF24L01_FlushRx();
		}
	}
    return packet_period;
}

uint16_t initSHENQI()
{
	BIND_IN_PROGRESS;	// autobind protocol
	SHENQI_init();
	hopping_frequency_no = 0;
	packet_count=0;
	packet_period=100;
	return 1000;
}

#endif