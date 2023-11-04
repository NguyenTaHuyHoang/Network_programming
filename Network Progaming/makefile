# Khai báo các biến
CC = gcc # Trình biên dịch C
TARGET = Web_client # Tên của các tệp thực thi
URL = http://www.google.com # URL của trang web cần tải xuống

# Quy tắc mặc định để xây dựng các tệp thực thi
all: $(TARGET)

# Quy tắc để xây dựng chương trình khách hàng
Web_client: Web_client.c
	$(CC) Web_client.c -o Web_client

# Quy tắc để chạy chương trình khách hàng với URL đã cho
run: Web_client
	./Web_client $(URL) output.html

# Quy tắc để làm sạch các tệp đối tượng và thực thi
clean:
	rm -f *.o $(TARGET)
