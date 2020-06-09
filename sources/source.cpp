// Copyright 2018 Your Name <your_email>

#include <header.hpp>

#include <iostream>
#include <boost/asio.hpp>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <chrono>
#include <time.h>


using sock = boost::asio::ip::tcp::socket;
using acceptor = boost::asio::ip::tcp::acceptor;
using endpoint = boost::asio::ip::tcp::endpoint;
using input_ip = boost::asio::ip::address;
using io_context = boost::asio::io_context;
using streambuffer = boost::asio::streambuf;
const char ip_sock[] = "127.0.0.1";
const char ping_k[] = "ping_ok!";
const char serv_start[] = "server started...";
const char read_until_ex[] = "read_until: Resource temporarily unavailable";
const char login[] = "login";
const char ping[] = "ping";
const char list[] = "list";
const char client_list[] = "Client list : ";
const char valid_request[] = "Please enter a valid request";
const char connected[] = "you are connected, ";

struct Client {
public:
    sock my_socket;
    string login;
    time_t sec = 0;

    explicit Client(boost::asio::io_context *context) :
            my_socket(*context) {}
};

class Server {
public:
    boost::asio::streambuf buffer{};
    io_context context;
    vector<shared_ptr<Client>> my_vector;
    std::mutex block;
    bool client_list_chandeg = false;

    Server() {
        cout << serv_start << endl;
    }

    void Listening_socket() {
        while (true) {
            Client zero_Client(&context);

            endpoint ep(input_ip::from_string(ip_sock), 8001);
            acceptor acc(context, ep);
            acc.accept(zero_Client.my_socket);
            zero_Client.my_socket.non_blocking(true);
            block.lock();
            cout << "client connected!" << endl;
            zero_Client.sec = time(NULL);
            my_vector.push_back(make_shared<Client>(std::move(zero_Client)));
            client_list_chandeg = true;

            block.unlock();
        }
    }

    void Choose_request() {
        while (true) {
 std::this_thread::sleep_for(std::chrono_literals::operator ""ms(1)); //9900kf
            block.lock();
            if (!my_vector.empty()) {
                for (auto it = my_vector.begin(); it != my_vector.end(); ++it) {
                    if (client_list_chandeg == true) {
                        continue;
                    }
                    try {
                        if (!(*it)->my_socket.is_open()) {
                            cout << "Socket is closed!";
                            continue;
                        }

                        (*it)->my_socket.non_blocking(true);
                        Communication(it);

                        (*it)->sec = time(NULL);
                    }
                    catch (exception &e) {
                        if (string(e.what()) == read_until_ex) {
                            if ((time(NULL) - (*it)->sec) >= 5) {
                                (*it)->my_socket.close();
                            }
                        } else
                            (*it)->my_socket.close();
                    }
                }
                client_list_chandeg = false;

                for (vector<shared_ptr<Client>>::iterator itera = my_vector.begin();
				itera != my_vector.end();) {
                    if (!(*itera)->my_socket.is_open()) {
                        my_vector.erase(itera);
                    } else {
                        ++itera;
                    }
                }
            }
            block.unlock();
        }
    }

    string Erase_str(string a) {
        a = a.erase(a.size() - 1);
        return a;
    }

    void List(vector<shared_ptr<Client>>::iterator iter_list) {
        boost::asio::streambuf buffer_list{};
        std::iostream out(&buffer_list);
        out << client_list;

		for (vector<shared_ptr<Client>>::iterator it = my_vector.begin();
		it != my_vector.end();) {
            out << (*it)->login << endl;
            ++it;	
        }

        boost::asio::write((*iter_list)->my_socket, buffer_list);
    }

    void Ping(vector<shared_ptr<Client>>::iterator iter_ping) {
        std::this_thread::sleep_for(std::chrono_literals::operator ""ms(1));
        boost::asio::read_until((*iter_ping)->my_socket, buffer, '\n');

        std::string ping(istreambuf_iterator<char>{&buffer},
                         istreambuf_iterator<char>{});
        ping = Erase_str(ping);

        cout << ping << endl;
        if (ping == ping_k) {
            std::ostream out(&buffer);
            out << ping_k << "\n";
            boost::asio::write((*iter_ping)->my_socket, buffer);
        }
    }

    void Login_record(vector<shared_ptr<Client>>::iterator iter) {
        (*iter)->my_socket.non_blocking(false);
        boost::asio::read_until((*iter)->my_socket, buffer, '\n');

        std::string name(istreambuf_iterator<char>{&buffer},
                         istreambuf_iterator<char>{});

        (*iter)->login = name;

        cout << "Client's name is " << name << endl;

        std::iostream out(&buffer);
        out << connected << name << "\n";
        boost::asio::write((*iter)->my_socket, buffer);
    }

    void Communication(vector<shared_ptr<Client>>::iterator client) {
        std::iostream out(&buffer);
        boost::asio::read_until((*client)->my_socket, buffer, '\n');

        std::string request(istreambuf_iterator<char>{&buffer},
                            istreambuf_iterator<char>{});

        request = Erase_str(request);

        if (request == list) {
            cout << "Client choose list" << endl;
            List(client);
       std::this_thread::sleep_for(std::chrono_literals::operator ""ms(1000));
        } else if (request == ping) {
            cout << "Client choose ping" << endl;

            out << ping_k << "\n";
            boost::asio::write((*client)->my_socket, buffer);
            Ping(client);
       std::this_thread::sleep_for(std::chrono_literals::operator ""ms(1000));
        } else if (request == login) {
            cout << "Client choose login" << endl;

            out << login << "\n";
            boost::asio::write((*client)->my_socket, buffer);
            Login_record(client);
       std::this_thread::sleep_for(std::chrono_literals::operator ""ms(1000));
        } else {
            std::iostream out(&buffer);
            out << valid_request << "\n";
            boost::asio::write((*client)->my_socket, buffer);
       std::this_thread::sleep_for(std::chrono_literals::operator ""ms(1000));
        }
    }

    void Start() {
        thread t1(&Server::Listening_socket, this);

        thread t2(&Server::Choose_request, this);

        t1.join();
        t2.join();
    }
};

int main() {
    Server s1;
    s1.Start();
    return 0;
}
