// Copyright 2020-2021 Rapyuta Robotics Co., Ltd and 09ubberboy90

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

// rclUE
#include "ROS2Subscriber.h"

#include <Msgs/ROS2Str.h>
#include <Msgs/ROS2Pose.h>
#include <Msgs/ROS2Img.h>
#include <Msgs/ROS2CameraInfo.h>
#include "TrackedObject.generated.h"

UCLASS()
class IMMERTWIN_API ATrackedObject : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATrackedObject();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
    UFUNCTION()
    void CameraMsgCallback(const UROS2GenericMsg* InMsg);

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UROS2NodeComponent* Node = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ObjectTopic = TEXT("/pose");
private:
    FVector OriginalScale;
};
