// Andre Mühlenbrock, 2020

#include "PointCloudRenderer.h"
#include "ImmerTwin/ImmerTwin.h"

// Sets default values
APointCloudRenderer::APointCloudRenderer()
{

    SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
    SetRootComponent(SceneComponent);

	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Just define how many points you want. You can define pointCount arbitrary as long as 
	// textureWidth * textureHeight is greater or equals pointCount:
	TextureWidth = 1280;
	TextureHeight = 720;
	PointCount = TextureWidth * TextureHeight;

	// Initialize data arrays:
	Positions = new float[TextureWidth * TextureHeight * 4];
	Colors = new uint8_t[TextureWidth * TextureHeight * 4];

	// // Now, we have to update the point positions & colors:
	// for (uint32_t x = 0; x < TextureWidth; ++x) {
	// 	for (uint32_t y = 0; y < TextureHeight; ++y) {
	// 		int id = x + y * TextureWidth;
	//
	// 		// Write some test data in the arrays:
	// 		Positions[id * 4 + 0] = float(x) / TextureWidth * 1000 - 500;  // < x
	// 		Positions[id * 4 + 1] = float(y) / TextureHeight * 1000 - 500; // < y
	// 		Positions[id * 4 + 2] = 0;
	// 		Positions[id * 4 + 3] = 1; // < could be removed, currently not used.
	//
	// 		Colors[id * 4 + 0] = 0;  // < b
	// 		Colors[id * 4 + 1] = uint8_t(float(y) / TextureHeight * 255); // < g
	// 		Colors[id * 4 + 2] = uint8_t(float(x) / TextureWidth * 255);									      // < r
	// 		Colors[id * 4 + 3] = 10;									  // < a
	// 	}
	// }

	// Precalculate a part of the sine function to avoid these costly calls in the tick function 
	// for every point:
	Precalc = new float[TextureWidth];
	for (uint32 i = 0; i < TextureWidth; ++i) {
		Precalc[i] = sin(i / float(TextureWidth) * 3.14159f * 2);
	}

	Region = FUpdateTextureRegion2D(0, 0, 0, 0, TextureWidth, TextureHeight);

    Node = CreateDefaultSubobject<UROS2NodeComponent>(TEXT("ROS2NodeComponent"));

    // these parameters can be change from BP
    Node->Name = TEXT("subscriber_node");
    Node->Namespace = TEXT("cpp");

}
void APointCloudRenderer::CameraMsgCallback(const UROS2GenericMsg* InMsg)
{
    if (const UROS2CameraInfoMsg* camMsg = Cast<UROS2CameraInfoMsg>(InMsg))
    {
        FROSCameraInfo msg;
        camMsg->GetMsg(msg);
        Camera_K = msg.K;
    }
}

void APointCloudRenderer::ImageMsgCallback(const UROS2GenericMsg* InMsg)
{
    if (const UROS2ImgMsg* imgMsg = Cast<UROS2ImgMsg>(InMsg))
    {
        imgMsg->GetMsg(Image);
    }
}

void APointCloudRenderer::DepthMsgCallback(const UROS2GenericMsg* InMsg)
{
    if (const UROS2ImgMsg* imgMsg = Cast<UROS2ImgMsg>(InMsg))
    {
        imgMsg->GetMsg(Depth);
    }
}
float APointCloudRenderer::BinToFloat(const int32 X)
{
    union {
        int32  X;
        float  F;
    } Temp;
    Temp.X = X;
    return Temp.F;
}

// Just the destructor:
APointCloudRenderer::~APointCloudRenderer()
{
	delete[] Positions;
	delete[] Colors;
	delete[] Precalc;
}

/**
 * Just a helper method to set a texture for a user variable because the UNiagaraComponent 
 * has no direct way to edit a texture variable compared to float, vectors, ...
 */

void APointCloudRenderer::SetNiagaraVariableTexture(class UNiagaraComponent* niagara, FString variableName, UTexture* texture) {
	if (!niagara || !texture)
		return;

	FNiagaraUserRedirectionParameterStore& overrideParameters = niagara->GetOverrideParameters();
	FNiagaraVariable niagaraVariable = FNiagaraVariable(FNiagaraTypeDefinition(UNiagaraDataInterfaceTexture::StaticClass()), *variableName);

	UNiagaraDataInterfaceTexture* dataInterface = (UNiagaraDataInterfaceTexture*)overrideParameters.GetDataInterface(niagaraVariable);
	dataInterface->SetTexture(texture);
}


// Called when the game starts or when spawned
void APointCloudRenderer::BeginPlay()
{
	Super::BeginPlay();

    auto Transform = SceneComponent->GetComponentTransform();
	// Initialize the Niagara System:
	rendererInstance = UNiagaraFunctionLibrary::SpawnSystemAttached(
		PointCloudRenderer,
		SceneComponent,
		FName(),
		Transform.GetLocation(),
		FRotator(0,0,0),
		EAttachLocation::SnapToTarget,
		true
	);
	rendererInstance->SetCastShadow(true);

	// // Create dynamic texture for position:
	PositionTexture = UTexture2D::CreateTransient(TextureWidth, TextureHeight, PF_A32B32G32R32F, "PositionData");
	PositionTexture->Filter = TF_Nearest;
	PositionTexture->UpdateResource();
	//
	// Create dynamic texture for color:
	ColorTexture = UTexture2D::CreateTransient(TextureWidth, TextureHeight, PF_B8G8R8A8, "ColorTexture");
	ColorTexture->Filter = TF_Nearest;
	ColorTexture->UpdateResource();

	// Set the niagara system user variables:
	SetNiagaraVariableTexture(rendererInstance, "User.PositionTexture", PositionTexture);
	SetNiagaraVariableTexture(rendererInstance, "User.ColorTexture", ColorTexture);

	rendererInstance->SetVariableInt("User.TextureWidth", TextureWidth);
	rendererInstance->SetVariableInt("User.TextureHeight", TextureHeight);
	rendererInstance->SetVariableInt("User.PointCount", PointCount);

	Runtime = 0;

    Node->Init();

    ROS2_CREATE_SUBSCRIBER(Node, this, CameraInfo, UROS2CameraInfoMsg::StaticClass(), &APointCloudRenderer::CameraMsgCallback);
    ROS2_CREATE_SUBSCRIBER(Node, this, ImageTopic, UROS2ImgMsg::StaticClass(), &APointCloudRenderer::ImageMsgCallback);
    ROS2_CREATE_SUBSCRIBER(Node, this, DepthTopic, UROS2ImgMsg::StaticClass(), &APointCloudRenderer::DepthMsgCallback);
    GetWorldTimerManager().SetTimer(MemberTimerHandle, this, &APointCloudRenderer::BuildPointCloud, .1f, true, 1.0);

}

// Called every frame
// void APointCloudRenderer::Tick(float DeltaTime)
// {
// 	// Super::Tick(DeltaTime);
//     TickCount++;
//     if (TickCount % 6 != 0)
//     {
//         return;
//     }
//     TickCount=0;
// 	Runtime += DeltaTime*6;
//
//     BuildPointCloud();
// }

void APointCloudRenderer::BuildPointCloud()
{
    if (Image.Data.Num() == 0)
        return;
    if (Depth.Data.Num() == 0)
        return;
    if (Camera_K.Num() == 0)
        return;
    if (Image.Width != Depth.Width || Image.Height != Depth.Height)
    {
        UE_LOG_WITH_INFO_NAMED(LogImmerTwin, Log, TEXT("Image and Depth Size do not match"));
        return;
    }
    const int w = Image.Width;  
    const int h = Image.Height;

    const double Cx = Camera_K[2];
    const double Cy = Camera_K[5];
    const double Fx_Inv = 1.0 / Camera_K[0];
    const double Fy_Inv = 1.0 / Camera_K[4];
    const float OriginalZStart = ZStart; 
    const float OriginalZEnd = ZEnd; 

    if (XEnd < 0)
        XEnd = w;
    if (YEnd < 0)
        YEnd = h;
    if (ZStart < 0)
        ZStart = std::numeric_limits<float>::max();

    for (int u = XStart; u < XEnd; u+=SkippedPixels)
    {
        for (int v = YStart; v < YEnd; v+=SkippedPixels)
        {
            const int SpriteID = (u + v * TextureWidth)*4;
            const auto Pixel_Loc = v * w * 4 + u * 4;

            
        	Colors[SpriteID + 3] = 0; // Set all point to be transparent unless we find them
            Positions[SpriteID + 2] = -10000; // dont knwo why this is needed as unvalid depth should stay transparent. But without it we get leftover artifact from movement
            if (Pixel_Loc >= Depth.Data.Num())
            {
                continue;
            }
            // const int32 z = Depth.Data[Pixel_Loc + 3] << 24 | Depth.Data[Pixel_Loc + 2] << 16 | Depth.Data[
            //                      Pixel_Loc + 1] << 8 | Depth.Data[Pixel_Loc];
            // const float ScaledZ = BinToFloat(z) * ZScale;

            const uint16 z = Depth.Data[Pixel_Loc + 1] << 8 | Depth.Data[Pixel_Loc];
            const float ScaledZ = z * ZScale;

        	if (std::isnan(ScaledZ))
        	{
        		continue;
        	}
        	if (OriginalZStart < 0 && ScaledZ < ZStart)
        	{
        		ZStart = ScaledZ;
        	}
        	if (OriginalZEnd < 0 && ScaledZ > ZEnd)
        	{
        		ZEnd = ScaledZ;
        	}
        	if (ZStart <= ScaledZ && ScaledZ <= ZEnd)
        	{
        		const auto b = Image.Data[Pixel_Loc];
        		const auto g = Image.Data[Pixel_Loc + 1];
        		const auto r = Image.Data[Pixel_Loc + 2];
        		const auto a = Image.Data[Pixel_Loc + 3];
        		constexpr auto FloatConv = 1 / static_cast<float>(0xFF);
        		// UE_LOG_WITH_INFO_NAMED(LogTurtlebot3, Log, TEXT("%d %d %f"),std::numeric_limits<uint16>::max(), z, z);

        		const float x = ScaledZ * ((u - Cx) * Fx_Inv);
        		const float y = ScaledZ * ((v - Cy) * Fy_Inv);
        	    Colors[SpriteID + 0] = b;
        	    Colors[SpriteID + 1] = g;
        	    Colors[SpriteID + 2] = r;
        	    Colors[SpriteID + 3] = a;
                Positions[SpriteID + 0] = x;
                Positions[SpriteID + 1] = y;
                Positions[SpriteID + 2] = -ScaledZ;
        	}
        }
    }
    // Now, bring the data into the texture:
    PositionTexture->UpdateTextureRegions(0, 1, &Region, TextureWidth * 16, 16, (uint8*)Positions);
    ColorTexture->UpdateTextureRegions(0, 1, &Region, TextureWidth * 4, 4, (uint8*)Colors);

}
