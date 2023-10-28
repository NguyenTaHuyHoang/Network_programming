# Khai báo các biến
CC = gcc # Trình biên dịch C
TARGET = client # Tên của các tệp thực thi
URL = http://example.com # URL của trang web cần tải xuống

# Quy tắc mặc định để xây dựng các tệp thực thi
all: $(TARGET)

# Quy tắc để xây dựng chương trình khách hàng
client: client.c
	$(CC) client.c -o client

# Quy tắc để chạy chương trình khách hàng với URL đã cho
run: client
	./client $(URL) output.txt

# Quy tắc để làm sạch các tệp đối tượng và thực thi
clean:
	rm -f *.o $(TARGET)
