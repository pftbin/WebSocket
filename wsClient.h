#pragma once


#pragma warning(disable:4996) 

#include <iostream>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>
#include <string>

using websocketpp::lib::thread;
typedef websocketpp::client<websocketpp::config::asio_client> ws_client;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

typedef websocketpp::config::asio_client::message_type::ptr message_ptr;
#ifdef WIN32
typedef void(__cdecl *wsClientMessage) (websocketpp::connection_hdl hdl, void* parent, std::string msg);
typedef void(__cdecl *wsClientStatus) (websocketpp::connection_hdl hdl, void* parent, std::string status);
#else
typedef void(__attribute__((__cdecl__)) *wsClientMessage) (websocketpp::connection_hdl hdl, void* parent, std::string msg);
typedef void(__attribute__((__cdecl__)) *wsClientStatus) (websocketpp::connection_hdl hdl, void* parent, std::string status);
#endif

namespace wsEndpoint
{
    class connection_metadata {
    public:
        typedef websocketpp::lib::shared_ptr<connection_metadata> ptr;

        connection_metadata(int id, websocketpp::connection_hdl hdl, std::string uri, void *parent, wsClientMessage msgcallback, wsClientStatus statuscallback)
            : m_id(id)
            , m_hdl(hdl)
            , m_status("Connecting")
            , m_uri(uri)
            , m_server("N/A")
            , m_msgcallback(msgcallback)
            , m_statuscallback(statuscallback)
        {
            m_parent = parent;
        }

        void on_open(ws_client *client, websocketpp::connection_hdl hdl) {
            m_status = "Open";

            ws_client::connection_ptr con = client->get_con_from_hdl(hdl);
            m_server = con->get_response_header("Server");

            if (m_statuscallback != nullptr) m_statuscallback(hdl, m_parent, m_status);
        }

        // if connection failed, the function will be invoke.
        void on_fail(ws_client *client, websocketpp::connection_hdl hdl) {
            m_status = "Failed";

            ws_client::connection_ptr con = client->get_con_from_hdl(hdl);
            m_server = con->get_response_header("Server");
            m_error_reason = con->get_ec().message();

            if (m_statuscallback != nullptr) m_statuscallback(hdl, m_parent, m_status);
        }

        void on_close(ws_client *client, websocketpp::connection_hdl hdl) {
            m_status = "Closed";
            ws_client::connection_ptr con = client->get_con_from_hdl(hdl);
            std::stringstream s;
            m_error_reason = s.str();

            if (m_statuscallback != nullptr) m_statuscallback(hdl, m_parent, m_status);
        }

        void on_message(websocketpp::connection_hdl hdl, ws_client::message_ptr msg) {
            if (msg->get_opcode() == websocketpp::frame::opcode::text) {
                if (m_msgcallback != nullptr) m_msgcallback(hdl, m_parent, msg->get_payload());
            }
        }

        websocketpp::connection_hdl get_hdl() const {
            return m_hdl;
        }

        int get_id() const {
            return m_id;
        }

        std::string get_status() const {
            return m_status;
        }

        std::string get_uri() const {
            return m_uri;
        }

        void record_sent_message(std::string message) {
            m_messages.push_back(">> " + message);
        }


    private:
        int m_id;
        websocketpp::connection_hdl m_hdl;
        std::string m_status;
        std::string m_uri;
        std::string m_server;
        std::string m_error_reason;
        std::vector<std::string> m_messages;
        wsClientMessage m_msgcallback;
        wsClientStatus m_statuscallback;
        void *m_parent;
    };
    typedef std::map<int, connection_metadata::ptr> con_list;

	class wsClient {
	public:
		wsClient();
		~wsClient();

        int connect(std::string const & uri,  void *parent, wsClientMessage _msgcallback = nullptr, wsClientStatus _statuscallback = nullptr);
		void close(int id);

		void send(int id, std::string message);
		void show();
    private:
        ws_client m_wsEndPoint;

        con_list m_connection_list;
        int m_next_id;
        websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_threadWS;
	};
}

