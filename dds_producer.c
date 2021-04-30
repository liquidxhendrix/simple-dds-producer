#include "dds_producer.h"

DDS_DomainParticipant *participant = NULL;

DDS_Publisher *publisher = NULL;
DDS_Topic *topic_send = NULL;
DDS_DataWriter *writer = NULL;
ShapeTypeDataWriter *ShapeType_writer = NULL;
struct DDS_Duration_t send_period = {1,0};
int count_writer = 0;  

DDS_Subscriber *subscriber = NULL;
DDS_Topic *topic_read = NULL;
struct DDS_DataReaderListener reader_listener =
DDS_DataReaderListener_INITIALIZER;
DDS_DataReader *reader = NULL;
int count_reader = 0;
struct DDS_Duration_t poll_period = {1,0};

int domain_id = 0;
int sample_count = 0; /* infinite loop */
char *colorkey="blue";

int main(int argc, char** argv) {
      printf("Hello, World! From Client\n");



    if (argc >= 2) {
        colorkey=argv[1];
    }

    if (argc >= 3) {
        domain_id = atoi(argv[2]);
    }
    if (argc >= 4) {
        sample_count = atoi(argv[3]);
    }

    /* Uncomment this to turn on additional logging
    NDDS_Config_Logger_set_verbosity_by_category(
        NDDS_Config_Logger_get_instance(),
        NDDS_CONFIG_LOG_CATEGORY_API, 
        NDDS_CONFIG_LOG_VERBOSITY_STATUS_ALL);
    */

    return producer_main(domain_id, sample_count);

}

void ShapeTypeListener_on_requested_deadline_missed(
    void* listener_data,
    DDS_DataReader* reader,
    const struct DDS_RequestedDeadlineMissedStatus *status)
{
}

void ShapeTypeListener_on_requested_incompatible_qos(
    void* listener_data,
    DDS_DataReader* reader,
    const struct DDS_RequestedIncompatibleQosStatus *status)
{
}

void ShapeTypeListener_on_sample_rejected(
    void* listener_data,
    DDS_DataReader* reader,
    const struct DDS_SampleRejectedStatus *status)
{
}

void ShapeTypeListener_on_liveliness_changed(
    void* listener_data,
    DDS_DataReader* reader,
    const struct DDS_LivelinessChangedStatus *status)
{
}

void ShapeTypeListener_on_sample_lost(
    void* listener_data,
    DDS_DataReader* reader,
    const struct DDS_SampleLostStatus *status)
{
}

void ShapeTypeListener_on_subscription_matched(
    void* listener_data,
    DDS_DataReader* reader,
    const struct DDS_SubscriptionMatchedStatus *status)
{
}

void ShapeTypeListener_on_data_available(
    void* listener_data,
    DDS_DataReader* reader)
{
    ShapeTypeDataReader *ShapeType_reader = NULL;
    struct ShapeTypeSeq data_seq = DDS_SEQUENCE_INITIALIZER;
    struct DDS_SampleInfoSeq info_seq = DDS_SEQUENCE_INITIALIZER;
    DDS_ReturnCode_t retcode;
    int i;

    ShapeType_reader = ShapeTypeDataReader_narrow(reader);
    if (ShapeType_reader == NULL) {
        fprintf(stderr, "DataReader narrow error\n");
        return;
    }

    retcode = ShapeTypeDataReader_take(
        ShapeType_reader,
        &data_seq, &info_seq, DDS_LENGTH_UNLIMITED,
        DDS_ANY_SAMPLE_STATE, DDS_ANY_VIEW_STATE, DDS_ANY_INSTANCE_STATE);
    if (retcode == DDS_RETCODE_NO_DATA) {
        return;
    } else if (retcode != DDS_RETCODE_OK) {
        fprintf(stderr, "take error %d\n", retcode);
        return;
    }

    for (i = 0; i < ShapeTypeSeq_get_length(&data_seq); ++i) {
        if (DDS_SampleInfoSeq_get_reference(&info_seq, i)->valid_data) {
            printf("Received data\n");
            ShapeTypeTypeSupport_print_data(
                ShapeTypeSeq_get_reference(&data_seq, i));
        }
    }

    retcode = ShapeTypeDataReader_return_loan(
        ShapeType_reader,
        &data_seq, &info_seq);
    if (retcode != DDS_RETCODE_OK) {
        fprintf(stderr, "return loan error %d\n", retcode);
    }
}

/* Delete all entities */
static int publisher_shutdown(
    DDS_DomainParticipant *participant)
{
    DDS_ReturnCode_t retcode;
    int status = 0;

    if (participant != NULL) {
        retcode = DDS_DomainParticipant_delete_contained_entities(participant);
        if (retcode != DDS_RETCODE_OK) {
            fprintf(stderr, "delete_contained_entities error %d\n", retcode);
            status = -1;
        }

        retcode = DDS_DomainParticipantFactory_delete_participant(
            DDS_TheParticipantFactory, participant);
        if (retcode != DDS_RETCODE_OK) {
            fprintf(stderr, "delete_participant error %d\n", retcode);
            status = -1;
        }
    }

    /* RTI Data Distribution Service provides finalize_instance() method on
    domain participant factory for people who want to release memory used
    by the participant factory. Uncomment the following block of code for
    clean destruction of the singleton. */
    /*
    retcode = DDS_DomainParticipantFactory_finalize_instance();
    if (retcode != DDS_RETCODE_OK) {
        fprintf(stderr, "finalize_instance error %d\n", retcode);
        status = -1;
    }
    */

    return status;
}

/* Delete all entities */
static int subscriber_shutdown(
    DDS_DomainParticipant *participant)
{
    DDS_ReturnCode_t retcode;
    int status = 0;

    if (participant != NULL) {
        retcode = DDS_DomainParticipant_delete_contained_entities(participant);
        if (retcode != DDS_RETCODE_OK) {
            fprintf(stderr, "delete_contained_entities error %d\n", retcode);
            status = -1;
        }

        retcode = DDS_DomainParticipantFactory_delete_participant(
            DDS_TheParticipantFactory, participant);
        if (retcode != DDS_RETCODE_OK) {
            fprintf(stderr, "delete_participant error %d\n", retcode);
            status = -1;
        }
    }

    /* RTI Data Distribution Service provides the finalize_instance() method on
    domain participant factory for users who want to release memory used
    by the participant factory. Uncomment the following block of code for
    clean destruction of the singleton. */
    /*
    retcode = DDS_DomainParticipantFactory_finalize_instance();
    if (retcode != DDS_RETCODE_OK) {
        fprintf(stderr, "finalize_instance error %d\n", retcode);
        status = -1;
    }
    */

    return status;
}

int producer_main(int domainId, int sample_count)
{

    //Common
    ShapeType *instance = NULL;
    DDS_ReturnCode_t retcode;
    DDS_InstanceHandle_t instance_handle = DDS_HANDLE_NIL;
    const char *type_name = NULL;

    //--> INIT DATA WRITER START

    /* To customize participant QoS, use 
    the configuration file USER_QOS_PROFILES.xml */
    participant = DDS_DomainParticipantFactory_create_participant(
        DDS_TheParticipantFactory, domainId, &DDS_PARTICIPANT_QOS_DEFAULT,
        NULL /* listener */, DDS_STATUS_MASK_NONE);
    if (participant == NULL) {
        fprintf(stderr, "create_participant error\n");
        publisher_shutdown(participant);
        return -1;
    }

    /* To customize publisher QoS, use 
    the configuration file USER_QOS_PROFILES.xml */
    publisher = DDS_DomainParticipant_create_publisher(
        participant, &DDS_PUBLISHER_QOS_DEFAULT, NULL /* listener */,
        DDS_STATUS_MASK_NONE);
    if (publisher == NULL) {
        fprintf(stderr, "create_publisher error\n");
        publisher_shutdown(participant);
        return -1;
    }

    /* Register type before creating topic */
    type_name = ShapeTypeTypeSupport_get_type_name();
    retcode = ShapeTypeTypeSupport_register_type(
        participant, type_name);
    if (retcode != DDS_RETCODE_OK) {
        fprintf(stderr, "register_type error %d\n", retcode);
        publisher_shutdown(participant);
        return -1;
    }

    /* To customize topic QoS, use 
    the configuration file USER_QOS_PROFILES.xml */
    topic_send = DDS_DomainParticipant_create_topic(
        participant, "SHAPE_SEND",
        type_name, &DDS_TOPIC_QOS_DEFAULT, NULL /* listener */,
        DDS_STATUS_MASK_NONE);
    if (topic_send == NULL) {
        fprintf(stderr, "create_topic error\n");
        publisher_shutdown(participant);
        return -1;
    }

    /* To customize data writer QoS, use 
    the configuration file USER_QOS_PROFILES.xml */
    writer = DDS_Publisher_create_datawriter(
        publisher, topic_send,
        &DDS_DATAWRITER_QOS_DEFAULT, NULL /* listener */, DDS_STATUS_MASK_NONE);
    if (writer == NULL) {
        fprintf(stderr, "create_datawriter error\n");
        publisher_shutdown(participant);
        return -1;
    }
    ShapeType_writer = ShapeTypeDataWriter_narrow(writer);
    if (ShapeType_writer == NULL) {
        fprintf(stderr, "DataWriter narrow error\n");
        publisher_shutdown(participant);
        return -1;
    }

    /* Create data sample for writing */
    instance = ShapeTypeTypeSupport_create_data_ex(DDS_BOOLEAN_TRUE);
    if (instance == NULL) {
        fprintf(stderr, "ShapeTypeTypeSupport_create_data error\n");
        publisher_shutdown(participant);
        return -1;
    }

    //<-- INIT DATA WRITER END 

    //--> INIT DATA READER FOR ECHO 

     /* To customize subscriber QoS, use 
    the configuration file USER_QOS_PROFILES.xml */
    subscriber = DDS_DomainParticipant_create_subscriber(
        participant, &DDS_SUBSCRIBER_QOS_DEFAULT, NULL /* listener */,
        DDS_STATUS_MASK_NONE);
    if (subscriber == NULL) {
        fprintf(stderr, "create_subscriber error\n");
        subscriber_shutdown(participant);
        return -1;
    }

    /* Register the type before creating the topic */
    type_name = ShapeTypeTypeSupport_get_type_name();
    retcode = ShapeTypeTypeSupport_register_type(participant, type_name);
    if (retcode != DDS_RETCODE_OK) {
        fprintf(stderr, "register_type error %d\n", retcode);
        subscriber_shutdown(participant);
        return -1;
    }

    /* To customize topic QoS, use 
    the configuration file USER_QOS_PROFILES.xml */
    topic_read = DDS_DomainParticipant_create_topic(
        participant, "SHAPE_ECHO",
        type_name, &DDS_TOPIC_QOS_DEFAULT, NULL /* listener */,
        DDS_STATUS_MASK_NONE);
    if (topic_read == NULL) {
        fprintf(stderr, "create_topic error\n");
        subscriber_shutdown(participant);
        return -1;
    }

    /* Set up a data reader listener */
    reader_listener.on_requested_deadline_missed  =
    ShapeTypeListener_on_requested_deadline_missed;
    reader_listener.on_requested_incompatible_qos =
    ShapeTypeListener_on_requested_incompatible_qos;
    reader_listener.on_sample_rejected =
    ShapeTypeListener_on_sample_rejected;
    reader_listener.on_liveliness_changed =
    ShapeTypeListener_on_liveliness_changed;
    reader_listener.on_sample_lost =
    ShapeTypeListener_on_sample_lost;
    reader_listener.on_subscription_matched =
    ShapeTypeListener_on_subscription_matched;
    reader_listener.on_data_available =
    ShapeTypeListener_on_data_available;

    /* To customize data reader QoS, use 
    the configuration file USER_QOS_PROFILES.xml */
    reader = DDS_Subscriber_create_datareader(
        subscriber, DDS_Topic_as_topicdescription(topic_read),
        &DDS_DATAREADER_QOS_DEFAULT, &reader_listener, DDS_STATUS_MASK_ALL);
    if (reader == NULL) {
        fprintf(stderr, "create_datareader error\n");
        subscriber_shutdown(participant);
        return -1;
    }

    // <---- INIT DATA READER END

    /* For a data type that has a key, if the same instance is going to be
    written multiple times, initialize the key here
    and register the keyed instance prior to writing */
    /*
    instance_handle = ShapeTypeDataWriter_register_instance(
        ShapeType_writer, instance);
    */
    /* Main loop */
    for (count_writer=0; (sample_count == 0) || (count_writer < sample_count); ++count_writer) {

        printf("Writing ShapeType, count %d\n", count_writer);

        /* Modify the data to be written here */

        instance->color=colorkey;
        instance->x=(double)rand()/(double)RAND_MAX*255;
        instance->y=(double)rand()/(double)RAND_MAX*255;
        instance->shapesize=(double)rand()/(double)RAND_MAX*20;

        /* Write data */
        retcode = ShapeTypeDataWriter_write(
            ShapeType_writer, instance, &instance_handle);
        if (retcode != DDS_RETCODE_OK) {
            fprintf(stderr, "write error %d\n", retcode);
        }

        NDDS_Utility_sleep(&send_period);

    }

    /*
    retcode = ShapeTypeDataWriter_unregister_instance(
        ShapeType_writer, instance, &instance_handle);
    if (retcode != DDS_RETCODE_OK) {
        fprintf(stderr, "unregister instance error %d\n", retcode);
    }
    */
    /* Delete data sample */
    retcode = ShapeTypeTypeSupport_delete_data_ex(instance, DDS_BOOLEAN_TRUE);
    if (retcode != DDS_RETCODE_OK) {
        fprintf(stderr, "ShapeTypeTypeSupport_delete_data error %d\n", retcode);
    }
    /* Cleanup and delete delete all entities */         
    return publisher_shutdown(participant);
}