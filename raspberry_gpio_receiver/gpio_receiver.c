#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <wiringPi.h>
#include <softTone.h>

#define PORT 51234       // 라즈베리파이에서 열 포트 번호
#define BUFSIZE 32
#define SPKR	6	// GPIO25 에 해당하는 wiringPi 번호



// 지정 가격보다 현재 코인가격이 하락일 때
void playSystemDownSound(){
	system("aplay /home/pi/raspberry_gpio_receiver/raspberry_gpio_receiver/down.wav &"); // 시스템 재생
}
void playDownTone(){
	softToneCreate(SPKR);
	softToneWrite(SPKR, 1760); // A6 (음)
	delay(400);
	softToneWrite(SPKR, 0); // 음 멈춤
}	
int ledControl_down(int gpio) {
    pinMode(gpio, OUTPUT);
	playDownTone();
	// playSystemDownSound();
	for (int i = 0; i < 3; i++) {
        digitalWrite(gpio, HIGH);
        delay(250);
        digitalWrite(gpio, LOW);
        delay(250);
    }
    return 0;
}

// 지정 가격보다 현재 코인가격이 상승일 때
void playSystemUpSound(){
	 system("aplay /home/pi/raspberry_gpio_receiver/raspberry_gpio_receiver/up.wav &"); // 시스템 재생
}
void playUpTone(){
	softToneCreate(SPKR);
	softToneWrite(SPKR, 587); // 음 : D5
	delay(400);
	softToneWrite(SPKR, 0); // 음 멈춤
}
int ledControl_up(int gpio){
	pinMode(gpio, OUTPUT);
	playUpTone();
	// playSystemUpSound();
	digitalWrite(gpio, 0);
	return 0;
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in serv_addr, cli_addr;
    char buffer[BUFSIZE];
    socklen_t cli_len = sizeof(cli_addr);

    // wiringPi 초기화
    if (wiringPiSetup() == -1) {
        perror("wiringPi setup failed");
        return 1;
    }

    // 소켓 생성
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket creation failed");
        return 1;
    }

    // 주소 구조체 설정
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY; // 모든 IP에서 연결 수락
    serv_addr.sin_port = htons(PORT);

    // 바인드
    if (bind(server_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind failed");
        close(server_fd);
        return 1;
    }

    // 리슨
    if (listen(server_fd, 1) < 0) {
        perror("listen failed");
        close(server_fd);
        return 1;
    }

    printf("Listening on port %d...\n", PORT);


while(1){
    // 클라이언트 연결 수락
    client_fd = accept(server_fd, (struct sockaddr*)&cli_addr, &cli_len);
    if (client_fd < 0) {
        perror("accept failed");
        close(server_fd);
        return 1;
    }
	
    // 메시지 수신
    memset(buffer, 0, BUFSIZE);
	read(client_fd, buffer, BUFSIZE - 1);
	printf("Received command: %s\n", buffer);

    int gpio_num = atoi(buffer); // 메시지가 GPIO 번호라면 정수로 변환

	switch(gpio_num){
		case 1:
			// down
			ledControl_down(1);
			break;
		case 2:
			// up
			ledControl_up(1);
			break;
		default:
			printf("Invalid GPIO number\n");
			break;
	}
	close(client_fd);
	
}
    close(server_fd);
    return 0;
}

