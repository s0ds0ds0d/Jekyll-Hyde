#include <iostream>
#include <stdio.h>
#include <stdint.h>


#pragma pack(push,1)
/* Windows 3.x bitmap file header */
typedef struct {
	char         filetype[2];   /* magic - always 'B' 'M' */
	unsigned int filesize;
	short        reserved1;
	short        reserved2;
	unsigned int dataoffset;    /* offset in bytes to actual bitmap data */
} file_header;

/* Windows 3.x bitmap full header, including file header */
typedef struct {
	file_header  fileheader;
	unsigned int headersize;
	int          width;
	int          height;
	short        planes;
	short        bitsperpixel;  /* we only support the value 24 here */
	unsigned int compression;   /* we do not support compression */
	unsigned int bitmapsize;
	int          horizontalres;
	int          verticalres;
	unsigned int numcolors;
	unsigned int importantcolors;
} bitmap_header;
#pragma pack(pop)


bool EncryptMsg(unsigned char* buf, unsigned int bufSize, char* msg, unsigned  int msgSize);
bool DecryptMsg(unsigned char* buf, unsigned int bufSize, char* msg, unsigned  int msgSize);
void IsHiddenMessage(char* msg, unsigned int msgSize);
void PrintMessage(char* msg, unsigned int msgSize);

int main()
{
	FILE *fp;
	
	bitmap_header header;
	static unsigned char *bitmap;
	unsigned int bitmapSize = 0;

	char imageName[255] = {0};
	printf("Podaj nazwe pliku do wczytania (*.bmp): ");
	scanf("%s", &imageName);
	
	if ((fp = fopen((const char*)imageName, "r+b")) == NULL) /////////////////////////////////////////////////////////////////////////////////////
	{
		printf("Nie mozna otworzyc pliku\n");
		getchar();
		getchar();
		return 1;
	}
	else
	{
		if (fread(&header, sizeof(bitmap_header), 1, fp) != NULL)
		{
			printf("==============================\n");
			printf("Dane z naglwoka:\n");
			printf("Calkowita liczba bajtow: %d\n", header.bitmapsize);
			printf("Szerokosc: %d\n", header.width);
			printf("Wysokosc:  %d\n", header.height);
			printf("==============================\n");
			//Total number of bytes in bitmap,eg, 100x100*3 (100x100 pixels, 1pix = 3 color channels)
			bitmapSize = header.bitmapsize;

			bitmap = (unsigned char *)malloc(sizeof(unsigned char)*bitmapSize);
			if (fread(bitmap, sizeof(unsigned char), bitmapSize, fp) != NULL)
			{
				printf("Wczytano bitmape z pliku\n");
				printf("==============================\n");
			}
			else
			{
				printf("Nie mozna odczytac bitmapy z pliku\n");
				printf("==============================\n");
			}
		}
	}

	unsigned int maxTextMessageLenght = (bitmapSize / 8)-1;
	char* message = (char*)malloc(sizeof(char) * maxTextMessageLenght);
	
	int choice = 0;

	printf("Wybierz typ operacji 0 lub 1:\n");
	printf("0. Ukryj teskt w bitmapie\n");
	printf("1. Odczytaj ukryta wiadomosc z bitmapy\n");
	
	scanf("%d", &choice);

	if ( !choice ) // 0
	{
		printf("Podaj tekst do ukrycia (max:%d znakow): ", maxTextMessageLenght);
		scanf("%s", message);
		message[maxTextMessageLenght - 1] = '\0';

		printf("Szyfrowanie wiadomosci: %s\n\n", message);
		if (!EncryptMsg(bitmap, bitmapSize, message, maxTextMessageLenght))
		{
			printf("Ukrywanie nie powiodlo sie.");
			printf("==============================\n");
		}
		
		fseek(fp, sizeof(bitmap_header), SEEK_SET);
		printf("Zapisano: %d bajtow\n\n", fwrite(bitmap, sizeof(unsigned char), bitmapSize, fp));
		printf("KONIEC\n");
		printf("==============================\n");
	}
	else // 1
	{
		printf("\n\nOdczytywanie wiadomosci:\n");
		char* messageOut = (char*)malloc(sizeof(char) * maxTextMessageLenght);
		DecryptMsg(bitmap, bitmapSize, messageOut, maxTextMessageLenght);
		
		PrintMessage(messageOut, maxTextMessageLenght);
		//IsHiddenMessage(messageOut, maxTextMessageLenght);
		free(messageOut);
		printf("==============================\n");
	}

	fclose(fp);
	free(bitmap);
	free(message);

	printf("\n");
	system("PAUSE");
}

void PrintMessage(char* msg, unsigned int msgSize)
{
	printf("OUTPUT: ");
	for (size_t i = 0; i < msgSize; i++)
	{
		if (msg[i] > 31 && msg[i] < 125)
		{
			printf("%c", msg[i]);
		}
	}
	printf("\n");
}

void IsHiddenMessage(char* msg, unsigned int msgSize)
{
	unsigned int counter = 0;
	unsigned int offset = 0;

	for (size_t i = 0; i < msgSize; i++)
	{
		if (msg[i] > 31 && msg[i] < 125)
		{
			++counter;
			if (counter > 7)
			{
				offset = i - counter;
				break;
			}
		}
		else
		{
			counter = 0;
		}
	}
	PrintMessage(&msg[offset], msgSize);
}

bool EncryptMsg(unsigned char* buf, unsigned int bufSize, char* msg, unsigned  int msgSize)
{
	char tmp = 0;
	//First clear last bits in each subpixel
	for (size_t i = 0; i < bufSize; i++)
	{
		tmp = buf[i];
		buf[i] = buf[i] & 0xFE;
	}
	
	if (msgSize+1 > bufSize)
	{
		printf("Error: Image to small to hide given message. Message = %d chars, Image can hide = %d chars", msgSize, bufSize / 8);
		return false;
	}

	// i index for char in msg array
	for (size_t i = 0; i < msgSize+1; i++)
	{
		char bitShift = 1;
		char currentChar = msg[i];
		for (size_t j = 0; j < 8; j++)
		{
			char checkBit = currentChar & bitShift;

			if (checkBit)
			{
				unsigned idx = (i * 8) + j;

				buf[idx] = buf[idx] ^ 0x01;
			}

			bitShift <<= 1;
		}
	}

	return true;
}

bool DecryptMsg(unsigned char* buf, unsigned int bufSize, char* msg, unsigned  int msgSize)
{
	//Inputem powinien byc tylko bufor z pikselami i jego rozmiar
	// i index for char in msg array
	for (size_t i = 0; i < msgSize; i++)
	{
		msg[i] = 0;
	}
	printf("\n");
	for (size_t i = 0; i < (msgSize); i++)
	{
		char bitShift = 1;
		char currentChar = 0;

		char checkBit = 0;
		for (size_t j = 0; j < 8; j++)
		{
			unsigned int idx = (i * 8) + j;
			char calc = buf[idx];

			checkBit = 0;
			checkBit = calc & 0x01;
			if (checkBit)
			{
				currentChar = currentChar | bitShift;
			}

			bitShift <<= 1;
		}

		msg[i] = currentChar;
	}
	printf("\n");
	

	return true;
}


