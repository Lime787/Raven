#include "serialization/messages.hpp"
#include <contexts.hpp>
#include <message_handler.hpp>
#include <moqt.hpp>
#include <moqt_client.hpp>
#include <msquic.h>
#include <serialization/serialization.hpp>

namespace rvn
{

void MessageHandler::operator()(ClientSetupMessage clientSetupMessage)
{
    utils::LOG_EVENT(std::cout, "Client Setup Message received: \n", clientSetupMessage);
    // Send Server Setup Message
    ServerSetupMessage serverSetupMessage;
    serverSetupMessage.selectedVersion_ = 0;

    streamState_.streamContext_->connectionState_.send_control_buffer(
    serialization::serialize(serverSetupMessage));
}

void MessageHandler::operator()(ServerSetupMessage serverSetupMessage)
{
    utils::LOG_EVENT(std::cout, "Server Setup Message received: \n", serverSetupMessage);
    MOQTClient& moqtClient =
    static_cast<MOQTClient&>(streamState_.streamContext_->moqtObject_);
    moqtClient.ravenConnectionSetupFlag_.store(true, std::memory_order_release);
}

void MessageHandler::operator()(SubscribeMessage subscribeMessage)
{
    utils::LOG_EVENT(std::cout, "Subscribe Message received: \n", subscribeMessage);
    TrackIdentifier trackIdentifier(subscribeMessage.trackNamespace_,
                                    subscribeMessage.trackName_);
    streamState_.connectionState_.add_track_alias(std::move(trackIdentifier),
                                                  subscribeMessage.trackAlias_);

    subscriptionManager_->add_subscription(streamState_.connectionState_.weak_from_this(),
                                           std::move(subscribeMessage));
}

void MessageHandler::operator()(StreamHeaderSubgroupObject streamHeaderSubgroupObject)
{
    // utils::LOG_EVENT(std::cout, "Stream Header Subgroup Object received: \n",
    //                  streamHeaderSubgroupObject);

    DataStreamState& dataStreamState = static_cast<DataStreamState&>(streamState_);
    dataStreamState.objectQueue_->enqueue(streamHeaderSubgroupObject);
}

void MessageHandler::operator()(StreamHeaderSubgroupMessage streamHeaderSubgroupMessage)
{
    // utils::LOG_EVENT(std::cout, "Stream Header Subgroup Message received: \n",
                    //  streamHeaderSubgroupMessage);

    DataStreamState& dataStreamState = static_cast<DataStreamState&>(streamState_);
    dataStreamState.set_header(std::move(streamHeaderSubgroupMessage));

    MOQTClient& moqtClient =
    static_cast<MOQTClient&>(streamState_.streamContext_->moqtObject_);

    moqtClient.dataStreamUserHandles_.enqueue(
    { dataStreamState.get_life_time_flag(),
      dataStreamState.streamHeaderSubgroupMessage_, dataStreamState.objectQueue_ });
}

} // namespace rvn
