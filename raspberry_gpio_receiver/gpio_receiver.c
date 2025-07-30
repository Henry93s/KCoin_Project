#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <wiringPi.h>

#define PORT 51234       // 라즈베리파이에서 열 포트 번호
#define BUFSIZE 32

int ledControl(int gpio) {
    pinMode(gpio, OUTPUT);
    for (int i = 0; i < 5; i++) {
        digitalWrite(gpio, HIGH);
        delay(1000);
        digitalWrite(gpio, LOW);
        delay(1000);
    }
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
    if (gpio_num >= 0 && gpio_num <= 29) {
        ledControl(gpio_num);
    } else {
        printf("Invalid GPIO number\n");
    }
	close(client_fd);
	
}
    close(server_fd);
    return 0;
}

