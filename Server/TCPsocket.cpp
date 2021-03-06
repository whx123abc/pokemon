﻿#include "TCPsocket.h"

TCPsocket::TCPsocket(QObject *parent)
	: QObject(parent)
{
}

TCPsocket::~TCPsocket()
{
}

void TCPsocket::tell_server_disconnected()
{
	QString res = QString("disconnect****%1").arg(socketdescriptor);
	emit string_to_server_ready(res);
}

void TCPsocket::string_from_handler(const QString & str,int i)
{
	switch (i)
	{
	case 1:
		socket->write((str+QString("^()")).toUtf8());
		break;
	case 2:
		QString result = QString("debug****%1").arg(str);
		emit string_to_server_ready(result);
		break;
	}
	
}

void TCPsocket::string_to_handler()
{
	QString str = socket->readAll();
	emit string_to_handler_ready(str);
}

void TCPsocket::socket_init(const qintptr socketDescriptor)
{

	socket = new QTcpSocket(this);
	socket->setSocketDescriptor(socketDescriptor);
	connect(socket, &QTcpSocket::readyRead, this, &TCPsocket::string_to_handler);

	handler = new Handler(this);
	connect(this, &TCPsocket::string_to_handler_ready, handler, &Handler::get_string_from_socket);
	connect(handler, &Handler::string_to_socket_ready, this, &TCPsocket::string_from_handler);
	socketdescriptor = socketDescriptor;
	connect(socket, &QTcpSocket::disconnected, this, &TCPsocket::tell_server_disconnected);
	connect(socket, &QTcpSocket::disconnected,  handler,&Handler::user_disconnect);
}
