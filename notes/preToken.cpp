// all code needed before creating a token & payable functions

#include <eosio/eosio.hpp>
#include <eosio/contract.hpp>

using namespace eosio;

CONTRACT doggo : public contract
{
public:
    using contract::contract;
    doggo(name receiver, name code,  datastream<const char*> ds): contract(receiver, code, ds) {}


    ACTION insert(name owner, std::string dog_name, int age)
    {
        require_auth( owner );
        dog_index dogs(get_first_receiver(), get_first_receiver().value);
        dogs.emplace(owner, [&]( auto& row ) {
            row.id = dogs.available_primary_key ();
            row.dog_name = dog_name;
            row.age = age;
            row.owner = owner;
        });
        send_summary(owner, "inserted dog");
    }

    ACTION erase(int dog_id) 
    {
        dog_index dogs( get_self(), get_self().value);

        auto dog = dogs.get(dog_id, "Unable to find dog");
        require_auth(dog.owner);

        auto iterator = dogs.find(dog_id);
        dogs.erase(iterator);
        send_summary(dog.owner, "erased dog");
    }

    ACTION removeall(name owner)
    {
        dog_index dogs(get_first_receiver(), get_first_receiver().value);

        auto owner_index = dogs.get_index<"byowner"_n>();
        auto iterator = owner_index.find(owner.value);

        while(iterator != owner_index.end()){
            owner_index.erase(iterator);
            iterator = owner_index.find(owner.value);
        }
        send_summary(owner, "erased all his/her dogs");
    }

    ACTION notify(name owner, std::string msg)
    {
        // auth of actual contract; this action should only be able to be called from within contract itself
            // inline action call - call from within this contract to notify
            // in order for this to actually be an action that can be executed like that, we need to req the auth of the contract itself
        require_auth(get_self());
        
        // owner is person making the original fn call to insert, erase, etc...
            // need require_recipient in order to notify the user/owner that this inline action was done
            // sends receipt to original caller; no "unknown" inline actions are being called w/o them knowing
        require_recipient(owner);
    }

private:
    TABLE dog
    {
        int id;
        std::string dog_name;
        int age;
        name owner;

        uint64_t primary_key() const { return id; }
        uint64_t get_secondary_1() const { return owner.value; }
    };

    //permission_level,
    //code,
    //action,
    //data
    void send_summary(name owner, std::string message){
        action(
            // permission_level{get_self(),"active"_n},
            permission_level{get_self(),"active"_n},
            get_self(),
            "notify"_n,
            // std::make_tuple(owner, name{owner}.to_string() + message)
            std::make_tuple(owner, message)
        ).send();
    };

    typedef multi_index<"dogs"_n, dog, indexed_by<"byowner"_n, const_mem_fun<dog, uint64_t, &dog::get_secondary_1>>> dog_index;

};