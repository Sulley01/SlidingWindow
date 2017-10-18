# SlidingWindow
Simulasi sliding window memanfaatkan UDP Socket
## Petunjuk Pengunaan Program
1. Clone atau Zip repository
2. Jalankan command Make setelah diclone atau zip di ekstrak
3. Eksekusi file dengan format berikut:
	./sendfile <filename> <windowsize> <buffersize> <destination_ip> <destination_port>
	./recvfile <filename> <windowsize> <buffersize> <port>
	silahkan ubah parameter dalam tag <> sesuai dengan kebutuhan anda

## Cara Kerja


## Pembagian tugas
* Alvin Sullivan - 13515048
* Christopher Clement Andreas - 13515105
* Kevin - 13515138

## Jawaban Pertanyaan

1. Apa yang terjadi jika advertised window yang dikirim bernilai 0? Apa cara untuk
menangani​ ​ hal​ ​ tersebut?

	Jawab: 
	Saat ukuran dari advertised window menjadi 0 akan menyebabkan TCP berhenti mengirimkan data sehingga dapat mencegah terjadinya overflow pada buffer receiver meskipun bagian sender akan tetap memasukkan data kedalam buffer send.

	Kasus terburuk yang terjadi adalah dengan ukuran advertised window=0 maka sender tidak akan mengirim paket sementara receiver juga mengalami hal yang sama. Hal ini menyebabkan terjadinya situasi dead-lock.

	jika ukuran advertise window adalah 0 dan pada sender masih terdapat data yang perlu dikirim kedalam buffer sender, sender akan mengirim secara periodik 1 byte kepada receiver untuk memicu respons dari receiver.


2. Sebutkan field data yang terdapat TCP Header serta ukurannya, ilustrasikan, dan jelaskan kegunaan masing-masing field data tersebut!
	
	Jawab:
	TCP Header Secara General adala sebagai berikut
	* Source TCP port (2 Bytes)
		Field ini merupakan bagian yang mengidentifikasi port pengirim
	* Destination TCP port number (2 bytes)
		Field ini merupakan bagian yang mengidentifikasi port penerima
	* Sequence number (4 bytes)
	    * Dalam transmisi normal, Field menunjukkan byte awal dari data di segmen ini.
		* Jika SYN(Synchronise) Flag menunjukkan nilai 1 maka ini merupakan sequence number awal(Initial Sequence Number) dan byte pertama dari data adalah ISN+1.
		* Jika SYN(Synchronise) Flag menunjukkan nilai 0 maka ini merupakan akumlasi dari byte pertama untuk sesi saat ini.
	* Acknowledgment number (4 bytes)
		Ketika ACK bit telah diatur, segment ini telah dikenali sebagai Acknowledgment dan menyimpan nilai terkait destinasi selanjutnya yang akan dikirim.
	* TCP data offset (4 bits)
		Field ini menspesifikasikan 32-bit words dari data dalam TCP header. Ini menunjukkan dari mana data dimulai.
	* Reserved data (3 bits)
		Field ini akan bernilai nol karena akan digunakan untuk kedepannya.
	* Control flags (up to 9 bits)
		Field ini digunakan untuk indikasi komunikasi dari kontrol informasi. Biasanya terdiri dari 6 bit 
		* URG: indikasi bahwa field Urgent pointer signifikan
		* ACK: indikasi bahwa field Acknowledgement signifikan
		* PSH: Perintah untuk melakukan push ke aplikasi penerima
		* RST: Reset terhadap koneksi
		* SYN: Sinkronisasi sequence number
		* FIN: paket terakhir dari sender
		* NS: proteksi 
		* CWR: Reduksi konflik
		* ECE: Jika SYN=1 maka TCP peer adalah ECN, sedangkan saat SYN=0 indikasi konflik network
	* Window size (2 bytes)
		Field ini menunjukkan ukuran dari window penerima yang dapat diterima oleh pengirim.
	* TCP checksum (2 bytes)
		Field ini digunakan untuk error-checking dari header.
	* Urgent pointer (2 bytes)
		jika URG flag di set maka 2 byte akan digunakan sebagai offset dari sequence number
	* TCP optional data (0-40 bytes)
		Terdapat 2 kasus yang menentukan format dari option:
			* Single octet dari option-kind
			* octet dari option-kind, octet dari option-length, dan octet dari option-data sebenarnya.
	* Padding
		Jika option field bukan kelipatan panjang dari 32 bit, sehingga akan ditambahkan 0 ke dalam field ini sehingga akan memenuhi 32 bit.
	* Data
		ukuran byte data yang akan dikirim dalam segment.

	TCP Header / format segment yang digunakan dalam tugas besar ini adalah 

	Field                |       Function	                                    |    size(byte)   |
	---------------------|------------------------------------------------------|-----------------| 
	SOH(Start of Header) | Posisi mulai segmen 									|		1		  |
	Sequence number      | Nomor segment									    |		4         |
	STX(Start of Text)   | Penanda mulainya data								|		1         |
	Data                 | Elemen yang akan dikirim								|		1         |
	ETX(End of Text)     | Penandan berakhirnya data                            |		1         |
	Checksum             | Flag untuk mengecek terjadinya error saat pengiriman |		1		  |


Ilustrasi TCP Header


SOH(0x1)  | Sequence Number | STX(0x2)  | Data    | ETX (0x3) | checksum |
----------|-----------------|-----------|---------|-----------|----------|
1 byte    |   4 byte        | 1 byte    |  1 byte |   1 byte  |   1 byte |

Reference:
http://www.tcpipguide.com/free/t_TCPMessageSegmentFormat-3.htm
http://www.freesoft.org/CIE/Course/Section4/8.htm
http://www.inetdaemon.com/tutorials/internet/tcp/3-way_handshake.shtml