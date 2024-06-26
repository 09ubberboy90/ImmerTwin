// Copyright 2020-2021 Rapyuta Robotics Co., Ltd and 09ubberboy90


#include "TrackedObject.h"

#include "ImmerTwin/ImmerTwin.h"


// Sets default values
ATrackedObject::ATrackedObject()
{
    Node = CreateDefaultSubobject<UROS2NodeComponent>(TEXT("ROS2NodeComponent"));

    // these parameters can be change from BP
    Node->Name = TEXT("subscriber_node");
    Node->Namespace = TEXT("cpp");

}

// Called when the game starts or when spawned
void ATrackedObject::BeginPlay()
{
	Super::BeginPlay();
    Node->Init();

    ROS2_CREATE_SUBSCRIBER(Node, this, ObjectTopic, UROS2PoseMsg::StaticClass(), &ATrackedObject::CameraMsgCallback);
    OriginalScale = this->GetActorScale3D();

}
void ATrackedObject::CameraMsgCallback(const UROS2GenericMsg* InMsg)
{
    const UROS2PoseMsg* camMsg = Cast<UROS2PoseMsg>(InMsg);
    if (camMsg)
    {
        FROSPose msg;
        camMsg->GetMsg(msg);
        FTransform tf(msg.Orientation.Rotator().Quaternion(), msg.Position, OriginalScale);
        this->SetActorTransform(tf);
    }
}
