# Khai báo các biến
CC = gcc # Trình biên dịch C
TARGET = 20120482_20120615 # Tên của các tệp thực thi
URL = http://www.google.com.vn # URL của trang web cần tải xuống

# Quy tắc mặc định để xây dựng các tệp thực thi
all: $(TARGET)

# Quy tắc để xây dựng chương trình khách hàng
20120482_20120615: 20120482_20120615.c
	$(CC) 20120482_20120615.c -o 20120482_20120615

# Quy tắc để chạy chương trình khách hàng với URL đã cho
run: 20120482_20120615
	./20120482_20120615 $(URL) output.html

# Quy tắc để làm sạch các tệp đối tượng và thực thi
clean:
	rm -f *.o $(TARGET)
