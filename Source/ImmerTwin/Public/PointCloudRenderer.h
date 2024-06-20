// Andre Mühlenbrock, 2020

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Runtime/Engine/Classes/Components/SceneComponent.h"

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

#include "PointCloudRenderer.generated.h"

UCLASS()
class IMMERTWIN_API APointCloudRenderer : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APointCloudRenderer();
	~APointCloudRenderer();
    void SetNiagaraVariableTexture(UNiagaraComponent* niagara, FString variableName, UTexture* texture);

    // The niagara system which should be used for rendering:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	UNiagaraSystem* PointCloudRenderer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	UTexture2D* PositionTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
    UTexture2D* ColorTexture;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
    int Scale = 100;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    class USceneComponent* SceneComponent;


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
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DebugConfig")
    int SkippedPixels = 1;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DebugConfig")
    float ZScale = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "X")
    int XStart = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "X")
    int XEnd = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Y")
    int YStart = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Y")
    int YEnd = -1;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Z")
    float ZStart = -1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Z")
    float ZEnd = -1;

private:
	// The instance of the niagara system:
	UNiagaraComponent* rendererInstance;

	// Textures to transfer the data to the niagara system:

	// further data:
	uint32_t TextureWidth;
	uint32_t TextureHeight;
	uint32_t PointCount;

	// The data which has to be written into the textures:
	float* Positions;
    uint8_t* Colors;
    int TickCount;

	// precalculated part of the sine function:
	float* Precalc;

	// Runtime of the game:
	float Runtime;

	// Definition of the region which should be updated:
	FUpdateTextureRegion2D Region;

    TArray<double> Camera_K;
    FROSImg Image;
    FROSImg Depth;
    
    FTimerHandle MemberTimerHandle;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	// virtual void Tick(float DeltaTime) override;
    void BuildPointCloud();
};
