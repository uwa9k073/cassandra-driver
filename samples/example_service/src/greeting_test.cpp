#include <greeting.hpp>

#include <userver/utest/utest.hpp>

using cassandra_driver_example::UserType;

UTEST(SayHelloTo, Basic) {
    EXPECT_EQ(cassandra_driver_example::SayHelloTo("Developer", UserType::kFirstTime), "Hello, Developer!\n");
    EXPECT_EQ(cassandra_driver_example::SayHelloTo({}, UserType::kFirstTime), "Hello, unknown user!\n");

    EXPECT_EQ(cassandra_driver_example::SayHelloTo("Developer", UserType::kKnown), "Hi again, Developer!\n");
    EXPECT_EQ(cassandra_driver_example::SayHelloTo({}, UserType::kKnown), "Hi again, unknown user!\n");
}