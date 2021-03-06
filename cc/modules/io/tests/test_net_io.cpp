#include "cc/modules/io/tests/test_helper.h"

TEST_CASE("NET IO 3PC, interactive", "[rosetta][io]") {
  int parties = 3;
  size_t size = 100;
  vector<int64_t> vi64_send;
  vector<uint64_t> vu64_send;
  rand_vec(vi64_send, size);
  rand_vec(vu64_send, size);

  auto run_case = [&](int party) {
    vector<string> ips;
    for (int i = 0; i < parties; i++) {
      ips.push_back("127.0.0.1");
    }
    shared_ptr<IO> io = make_shared<IO>(parties, party, 1, 8864, ips);
    io->set_server_cert("certs/server-nopass.cert");
    io->set_server_prikey("certs/server-prikey");
    io->init();
    io->sync();

    ////////////////////////// BEGIN
    if (party == 0) {
      vector<int64_t> vi64_recv(size);
      io->recv(1, vi64_recv, vi64_recv.size());
      REQUIRE(vi64_recv.size() == vi64_send.size());
      for (int i = 0; i < size; i += size / 10) {
        REQUIRE(vi64_recv[i] == vi64_send[i]);
      }
    } else if (party == 1) {
      io->send(0, vi64_send, vi64_send.size());
      io->send(2, vu64_send, vu64_send.size());
    } else if (party == 2) {
      vector<int64_t> vu64_recv(size);
      io->recv(1, vu64_recv, vu64_recv.size());
      REQUIRE(vu64_recv.size() == vu64_send.size());
      for (int i = 0; i < size; i += size / 10) {
        REQUIRE(vu64_recv[i] == vu64_send[i]);
      }
    }
    ////////////////////////// END

    io->sync();
    sleep(1);
    io->close();
  };

  vector<thread> threads(parties);
  for (int i = 0; i < parties; i++) {
    threads[i] = thread(run_case, i);
  }
  for (int i = 0; i < parties; i++) {
    threads[i].join();
  }
  sleep(1);
}

TEST_CASE("NET IO 3PC, broadcast", "[rosetta][io]") {
  int parties = 3;
  size_t size = 100;
  vector<int64_t> vi64_send;
  vector<uint64_t> vu64_send;
  rand_vec(vi64_send, size);
  rand_vec(vu64_send, size);

  auto run_case = [&](int party) {
    vector<string> ips;
    for (int i = 0; i < parties; i++) {
      ips.push_back("127.0.0.1");
    }
    shared_ptr<IO> io = make_shared<IO>(parties, party, 1, 9981, ips);
    io->set_server_cert("certs/server-nopass.cert");
    io->set_server_prikey("certs/server-prikey");
    io->init();
    io->sync();

    ////////////////////////// BEGIN
    if ((party == 0) || (party == 1)) {
      {
        vector<int64_t> vi64_recv(size);
        io->recv(2, vi64_recv, size);
        REQUIRE(vi64_recv.size() == vi64_send.size());
        for (int i = 0; i < size; i += size / 10) {
          REQUIRE(vi64_recv[i] == vi64_send[i]);
        }
      }
      {
        vector<int64_t> vu64_recv(size);
        io->recv(2, vu64_recv, size);
        REQUIRE(vu64_recv.size() == vu64_send.size());
        for (int i = 0; i < size; i += size / 10) {
          REQUIRE(vu64_recv[i] == vu64_send[i]);
        }
      }

    } else if (party == 2) {
      io->broadcast(vi64_send, size);
      io->broadcast(vu64_send, size);
    }
    ////////////////////////// END

    io->sync();
    sleep(1);
    io->close();
  };

  vector<thread> threads(parties);
  for (int i = 0; i < parties; i++) {
    threads[i] = thread(run_case, i);
  }
  for (int i = 0; i < parties; i++) {
    threads[i].join();
  }
  sleep(1);
}

TEST_CASE("NET IO MPC, random broadcast", "[rosetta][io]") {
  srand(time(NULL));
  int parties = rand() % 8 + 3; // [3,10]
  int send_party = rand() % parties;
  std::cout << "parties:" << parties << ", send_party id:" << send_party << std::endl;

  size_t size = 100;
  vector<int64_t> vi64_send;
  rand_vec(vi64_send, size);

  auto run_case = [&](int party) {
    vector<string> ips;
    for (int i = 0; i < parties; i++) {
      ips.push_back("127.0.0.1");
    }
    shared_ptr<IO> io = make_shared<IO>(parties, party, 1, 7749, ips);
    io->set_server_cert("certs/server-nopass.cert");
    io->set_server_prikey("certs/server-prikey");
    io->init();
    io->sync();

    ////////////////////////// BEGIN
    if (party == send_party) {
      io->broadcast(vi64_send, size);
    } else {
      vector<int64_t> vi64_recv(size);
      io->recv(send_party, vi64_recv, size);
      REQUIRE(vi64_recv.size() == vi64_send.size());
      for (int i = 0; i < size; i += size / 10) {
        REQUIRE(vi64_recv[i] == vi64_send[i]);
      }
    }
    ////////////////////////// END

    io->sync();
    sleep(1);
    io->close();
  };

  vector<thread> threads(parties);
  for (int i = 0; i < parties; i++) {
    threads[i] = thread(run_case, i);
  }
  for (int i = 0; i < parties; i++) {
    threads[i].join();
  }
  sleep(1);
}
