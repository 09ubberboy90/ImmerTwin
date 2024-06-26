// Copyright 2020-2021 Rapyuta Robotics Co., Ltd and 09ubberboy90

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraDataInterfaceTexture.h"
#include "Engine/Texture2D.h"
// rclUE
#include "ROS2Subscriber.h"
#include <Msgs/ROS2Str.h>
#include <Msgs/ROS2Img.h>
#include <Msgs/ROS2CameraInfo.h>
#include "Msgs/ROS2PointCloud2.h"

#include "PointCloudShaderBased.generated.h"

UCLASS()
class IMMERTWIN_API APointCloudShaderBased : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
    APointCloudShaderBased();
    static void SetNiagaraVariableTexture(UNiagaraComponent* niagara, FString variableName, UTexture* texture);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	UNiagaraSystem* PointCloudRenderer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	UTexture2D* PositionTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
    UTexture2D* ColorTexture;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
    int Scale = 10000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
    int PointCount = 10000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
    int NewPointCount = -1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX", meta = (ClampMin = "0.0", ClampMax = "2.0", UIMin = "0.0", UIMax = "2.0"))
    float ParticleSize=1;
    UPROPERTY(EditInstanceOnly)
    bool Reload;


    
    UFUNCTION()
    void CameraMsgCallback(const UROS2GenericMsg* InMsg);
    UFUNCTION()
    void ImageMsgCallback(const UROS2GenericMsg* InMsg);
    UFUNCTION()
    void DepthMsgCallback(const UROS2GenericMsg* InMsg);
    UFUNCTION()
    static float BinToFloat(const int32 X);

    UROS2NodeComponent* Node = nullptr;

    UPROPERTY(EditAnywhere, Category = "Images")
    FString ImageTopic = TEXT("/camera/color/image_raw");

    UPROPERTY(EditAnywhere, Category = "Images")
    FString DepthTopic = TEXT("/camera/aligned_depth_to_color/image_raw");

    UPROPERTY(EditAnywhere, Category = "Images")
    FString CameraInfo = TEXT("/camera/aligned_depth_to_color/camera_info");

    UPROPERTY(EditAnywhere, Category = "Rendering")
    bool CastShadow;

    // UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DebugConfig")
    // int SkippedPixels = 1;
    //
    // UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DebugConfig")
    // float ZScale = 1;
    //
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "X")
    int CropLeft = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "X")
    int CropRight = 0;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Y")
    int CropTop = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Y")
    int CropBottom = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
    int SkipPoints = 1;
    //
    // UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Z")
    // float ZStart = -1;
    // UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Z")
    // float ZEnd = -1;

private:
	// The instance of the niagara system:
	UNiagaraComponent* rendererInstance;
    class USceneComponent* SceneComponent;

	// Textures to transfer the data to the niagara system:

	// further data:
	uint32_t TextureWidth;
	uint32_t TextureHeight;
    uint32_t CroppedTextureWidth;
    uint32_t CroppedTextureHeight;

	// Definition of the region which should be updated:
	FUpdateTextureRegion2D Region;

    TArray<double> Camera_K;
    FROSImg Image;
    FROSImg Depth;
    
    FTimerHandle MemberTimerHandle;

    bool Initialized = false;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	void InitCloud();
    virtual void PostEditChangeProperty(FPropertyChangedEvent& e);
};
