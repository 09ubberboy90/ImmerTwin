// Copyright 2020-2021 Rapyuta Robotics Co., Ltd and 09ubberboy90


#include "PointCloudShaderBased.h"
#include "ImmerTwin/ImmerTwin.h"

// Sets default values
APointCloudShaderBased::APointCloudShaderBased()
{

    SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
    SetRootComponent(SceneComponent);
    
	// Just define how many points you want. You can define pointCount arbitrary as long as 
	// textureWidth * textureHeight is greater or equals pointCount:

    Node = CreateDefaultSubobject<UROS2NodeComponent>(TEXT("ROS2NodeComponent"));

    // these parameters can be change from BP
    Node->Name = TEXT("shader_node");
    Node->Namespace = TEXT("cpp");

}
void APointCloudShaderBased::CameraMsgCallback(const UROS2GenericMsg* InMsg)
{
    if (const UROS2CameraInfoMsg* camMsg = Cast<UROS2CameraInfoMsg>(InMsg))
    {
        FROSCameraInfo msg;
        camMsg->GetMsg(msg);
        TextureWidth = msg.Width;
        TextureHeight = msg.Height;
        Camera_K = msg.K;
        if (!Initialized) InitCloud();
    }
}

void APointCloudShaderBased::ImageMsgCallback(const UROS2GenericMsg* InMsg)
{
    if (!Initialized) return;
    if (const UROS2ImgMsg* imgMsg = Cast<UROS2ImgMsg>(InMsg))
    {
        imgMsg->GetMsg(Image);
        ColorTexture->UpdateTextureRegions(0, 1, &Region, TextureWidth * 4, 4, (uint8*)Image.Data.GetData());

    }
}

void APointCloudShaderBased::DepthMsgCallback(const UROS2GenericMsg* InMsg)
{
    if (!Initialized) return;
    if (const UROS2ImgMsg* imgMsg = Cast<UROS2ImgMsg>(InMsg))
    {
        imgMsg->GetMsg(Depth);
        // TArray<float> tmp = reinterpret_cast<TArray<float>>(Depth.Data);
        PositionTexture->UpdateTextureRegions(0, 1, &Region, TextureWidth * 2, 2, (uint8*)Depth.Data.GetData());

    }
}
float APointCloudShaderBased::BinToFloat(const int32 X)
{
    union {
        int32  X;
        float  F;
    } Temp;
    Temp.X = X;
    return Temp.F;
}


/**
 * Just a helper method to set a texture for a user variable because the UNiagaraComponent 
 * has no direct way to edit a texture variable compared to float, vectors, ...
 */

void APointCloudShaderBased::SetNiagaraVariableTexture(class UNiagaraComponent* niagara, FString variableName, UTexture* texture) {
	if (!niagara || !texture)
		return;

	FNiagaraUserRedirectionParameterStore& overrideParameters = niagara->GetOverrideParameters();
	FNiagaraVariable niagaraVariable = FNiagaraVariable(FNiagaraTypeDefinition(UNiagaraDataInterfaceTexture::StaticClass()), *variableName);

	UNiagaraDataInterfaceTexture* dataInterface = (UNiagaraDataInterfaceTexture*)overrideParameters.GetDataInterface(niagaraVariable);
	dataInterface->SetTexture(texture);
}

// Called when the game starts or when spawned
void APointCloudShaderBased::BeginPlay()
{
	Super::BeginPlay();
	// Initialize the Niagara System:

    Node->Init();

    ROS2_CREATE_SUBSCRIBER(Node, this, CameraInfo, UROS2CameraInfoMsg::StaticClass(), &APointCloudShaderBased::CameraMsgCallback);
    ROS2_CREATE_SUBSCRIBER(Node, this, ImageTopic, UROS2ImgMsg::StaticClass(), &APointCloudShaderBased::ImageMsgCallback);
    ROS2_CREATE_SUBSCRIBER(Node, this, DepthTopic, UROS2ImgMsg::StaticClass(), &APointCloudShaderBased::DepthMsgCallback);
}

void APointCloudShaderBased::InitCloud()
{

    rendererInstance = UNiagaraFunctionLibrary::SpawnSystemAttached(
    PointCloudRenderer,
    SceneComponent,
    FName(),
    UE::Math::TVector<double>(0,0,0),
    FRotator(0,0,0),
    EAttachLocation::SnapToTarget,
    true
    );

    CroppedTextureWidth = TextureWidth - CropLeft - CropRight;
    CroppedTextureHeight = TextureHeight - CropTop - CropBottom;
    PointCount = CroppedTextureWidth * CroppedTextureHeight;
    if (NewPointCount > 0)
    {
        PointCount = NewPointCount;
    }else
    {
        PointCount = PointCount / SkipPoints;
    }
    
    UE_LOG_WITH_INFO_NAMED(LogImmerTwin, Log, TEXT("%d %d %d %d"),CroppedTextureWidth, CroppedTextureHeight , CropLeft, CropRight);

    Region = FUpdateTextureRegion2D(0, 0, 0, 0, TextureWidth, TextureHeight);
    rendererInstance->SetCastShadow(CastShadow);

    // // Create dynamic texture for position:
    const FName PositionName = FName(GetFName().ToString() + FString("_position"));
    PositionTexture = UTexture2D::CreateTransient(TextureWidth, TextureHeight, PF_G16, PositionName);
    PositionTexture->MipGenSettings = TMGS_NoMipmaps;
    PositionTexture->Filter = TF_Nearest;
    PositionTexture->UpdateResource();
    //
    // Create dynamic texture for color:
    const FName ColorName = FName(GetFName().ToString() + FString("_color"));
    ColorTexture = UTexture2D::CreateTransient(TextureWidth, TextureHeight, PF_B8G8R8A8, ColorName);
    ColorTexture->MipGenSettings = TMGS_NoMipmaps;
    ColorTexture->Filter = TF_Nearest;
    ColorTexture->UpdateResource();

    // Set the niagara system user variables:
    SetNiagaraVariableTexture(rendererInstance, "User.PositionTexture", PositionTexture);
    SetNiagaraVariableTexture(rendererInstance, "User.ColorTexture", ColorTexture);

    float fCropLeft = CropLeft / static_cast<float>(TextureWidth);
    float fCropRight = (TextureWidth - CropRight) / static_cast<float>(TextureWidth);
    float fCropTop = CropTop / static_cast<float>(TextureHeight);
    float fCropBottom = (TextureHeight - CropBottom) / static_cast<float>(TextureHeight);

    rendererInstance->SetVariableFloat("User.CropLeft", fCropLeft);
    rendererInstance->SetVariableFloat("User.CropRight", fCropRight);
    rendererInstance->SetVariableFloat("User.CropTop", fCropTop);
    rendererInstance->SetVariableFloat("User.CropBottom", fCropBottom);
    UE_LOG_WITH_INFO_NAMED(LogImmerTwin, Log, TEXT("%f %f %f %f"), fCropLeft,fCropRight, fCropTop,fCropBottom);
    rendererInstance->SetVariableInt("User.SkippedLeft", CropLeft);
    rendererInstance->SetVariableInt("User.SkippedTop", CropTop);
    rendererInstance->SetVariableInt("User.TextureWidth", CroppedTextureWidth);
    rendererInstance->SetVariableInt("User.TextureHeight", CroppedTextureHeight);
    rendererInstance->SetVariableInt("User.PointCount", PointCount);
    rendererInstance->SetVariableInt("User.Scale", Scale);
    rendererInstance->SetVariableInt("User.SkipPoints", SkipPoints);
    rendererInstance->SetVariableFloat("User.Cx", Camera_K[2]);
    rendererInstance->SetVariableFloat("User.Cy", Camera_K[5]);
    rendererInstance->SetVariableFloat("User.FX", Camera_K[0]);
    rendererInstance->SetVariableFloat("User.ParticleSize", ParticleSize);
    UE_LOG_WITH_INFO_NAMED(LogImmerTwin, Log, TEXT("Cx:%f Cy:%f"),Camera_K[2], Camera_K[5]);

    Initialized = true;
    
}
void APointCloudShaderBased::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
    FName PropertyName = (e.Property != NULL) ? e.Property->GetFName() : NAME_None;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(APointCloudShaderBased, Reload)) {
        if (Reload)
        {
            InitCloud(); 
            Reload=false;
        }
    }
}
