#include "wsClient.h"
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <sstream>

namespace wsEndpoint
{	
    //static ws_client g_wsEndPoint;
    //typedef std::map<int, connection_metadata::ptr> con_list;
    //static con_list m_connection_list;
    //static int m_next_id = 0;//zwb修改，加入为0的初始值
    //static websocketpp::lib::shared_ptr<websocketpp::lib::thread> g_threadWS;

	wsClient::wsClient() {
        m_next_id = 0;
        m_wsEndPoint.clear_access_channels(websocketpp::log::alevel::all);
        m_wsEndPoint.clear_error_channels(websocketpp::log::elevel::all);

        m_wsEndPoint.init_asio();
        m_wsEndPoint.start_perpetual();

        m_threadWS.reset(new websocketpp::lib::thread(&ws_client::run, &m_wsEndPoint));

	}

	wsClient::~wsClient() {
        m_wsEndPoint.stop_perpetual();

		for (con_list::const_iterator it = m_connection_list.begin(); it != m_connection_list.end(); ++it) {
			if (it->second->get_status() != "Open") {
				// Only close open connections
				continue;
			}
			websocketpp::lib::error_code ec;
            m_wsEndPoint.close(it->second->get_hdl(), websocketpp::close::status::going_away, "", ec);
			if (ec) {
				//错误信息 ec.message() 
			}
		}

        m_threadWS->join();
	}

    int wsClient::connect(std::string const & uri, void *parent, wsClientMessage _msgcallback, wsClientStatus _statuscallback) {
		websocketpp::lib::error_code ec;

        ws_client::connection_ptr pConnection = m_wsEndPoint.get_connection(uri, ec);

		if (ec) {
			//std::cout << "> Connect initialization error: " << ec.message() << std::endl;
			return -1;
		}
		int new_id = m_next_id++;
		connection_metadata::ptr metadata_ptr;// = websocketpp::lib::make_shared<connection_metadata>(new_id, pConnection->get_handle(), uri);	
        metadata_ptr.reset( new connection_metadata(new_id, pConnection->get_handle(), uri, parent, _msgcallback, _statuscallback));
		m_connection_list[new_id] = metadata_ptr;
		pConnection->set_open_handler(websocketpp::lib::bind(
			&connection_metadata::on_open,
			metadata_ptr,
            &m_wsEndPoint,
			websocketpp::lib::placeholders::_1
		));
		pConnection->set_fail_handler(websocketpp::lib::bind(
			&connection_metadata::on_fail,
			metadata_ptr,
            &m_wsEndPoint,
			websocketpp::lib::placeholders::_1
		));
		pConnection->set_close_handler(websocketpp::lib::bind(
			&connection_metadata::on_close,
			metadata_ptr,
            &m_wsEndPoint,
			websocketpp::lib::placeholders::_1
		));
		pConnection->set_message_handler(websocketpp::lib::bind(
			&connection_metadata::on_message,
			metadata_ptr,
			websocketpp::lib::placeholders::_1,
			websocketpp::lib::placeholders::_2
		));

        m_wsEndPoint.connect(pConnection);

        return new_id;//zwb修改，原来是返回0
	}

	/*void close(websocketpp::close::status::value code, std::string reason) {
		websocketpp::lib::error_code ec;
		for (con_list::const_iterator it = m_connection_list.begin(); it != m_connection_list.end(); ++it) {
			if (it->second->get_status() != "Open") {
				// Only close open connections
				continue;
			g_wsEndPoint.close(it->second->get_hdl(), code, reason, ec);
		}
		
	}*/

	void wsClient::close(int id)
	{
		websocketpp::lib::error_code ec;

		con_list::iterator metadata_it = m_connection_list.find(id);
		if (metadata_it == m_connection_list.end()) {
			return;
		}

		if (metadata_it->second->get_status() == "Open")
		{
            websocketpp::close::status::value close_code = websocketpp::close::status::normal;
            m_wsEndPoint.close(metadata_it->second->get_hdl(), close_code, "Close", ec);
		}
	}

	void wsClient::send(int id, std::string message) {
		websocketpp::lib::error_code ec;

		
		con_list::iterator metadata_it = m_connection_list.find(id);
		if (metadata_it == m_connection_list.end()) {
			return;
		}

        m_wsEndPoint.send(metadata_it->second->get_hdl(), message, websocketpp::frame::opcode::text, ec);
		if (ec) {			
			return;
		}

        //metadata_it->second->record_sent_message(message);//zwb注释
		
	}

	void wsClient::show()
	{
		//std::cout << *g_wsClientConnection << std::endl;
	}
}
