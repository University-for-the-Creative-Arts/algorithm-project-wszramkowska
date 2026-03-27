#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "WFCManager.generated.h"

USTRUCT(BlueprintType)
struct FWFCTileData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMesh* TileMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SocketNorth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SocketEast;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SocketSouth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SocketWest;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString TileName;

	FWFCTileData()
		: TileMesh(nullptr)
		, SocketNorth(0)
		, SocketEast(0)
		, SocketSouth(0)
		, SocketWest(0)
		, TileName(TEXT("Default"))
	{
	}
};



USTRUCT(BlueprintType)
struct FWFCGridCell
{
	GENERATED_BODY()

	UPROPERTY()
	bool bIsCollapsed;

	UPROPERTY()
	TArray<FWFCTileData> PossibleTiles;

	UPROPERTY()
	FWFCTileData ChosenTile;

	UPROPERTY()
	int32 OptionsCount;

	UPROPERTY()
	int32 GridX;

	UPROPERTY()
	int32 GridY;

	UPROPERTY()
	FVector WorldLocation;

	UPROPERTY()
	AActor* SpawnedActor;

	FWFCGridCell()
		: bIsCollapsed(false)
		, OptionsCount(0)
		, GridX(0)
		, GridY(0)
		, WorldLocation(FVector::ZeroVector)
		, SpawnedActor(nullptr)
	{
	}
};

UCLASS()
class PROCEDURALSYSTEM_API AWFCManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWFCManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC|Grid Settings")
	int32 GridSizeX = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC|Grid Settings")
	int32 GridSizeY = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC|Grid Settings")
	float TileSize = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC|Tile Data")
	UDataTable* TileDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC|Generation")
	float GenerationSpeed = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC|Generation")
	bool bAutoGenerate = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFC|Debug")
	bool bShowDebugBoxes = true;
	
	UFUNCTION(BlueprintCallable, Category = "WFC")
	void StartGeneration();

	UFUNCTION(BlueprintCallable, Category = "WFC")
	void StopGeneration();

	UFUNCTION(BlueprintCallable, Category = "WFC")
	void ClearGrid();
	
private:
	
	TArray<FWFCGridCell> Grid;
	TArray<FWFCTileData> AllTiles;
	bool bIsGenerating;
	FTimerHandle GenerationTimerHandle;
	
	int32 GetGridIndex(int32 X, int32 Y) const;
	bool GetCellAt(int32 X, int32 Y, FWFCGridCell& OutCell) const;
	void SetCellAt(int32 X, int32 Y, const FWFCGridCell& NewCell);
	bool CanConnect(const FWFCTileData& Tile1, const FWFCTileData& Tile2, int32 Direction) const;
	
	void LoadTiles();
	void InitializeGrid();
	TArray<FWFCTileData> GetValidTiles(int32 X, int32 Y);
	bool FindCellWithFewestOptions(int32& OutX, int32& OutY);
	void CollapseCell(int32 X, int32 Y);
	void PropagateConstraints();
	void SpawnTile(int32 X, int32 Y, const FWFCTileData& TileData);
	void GenerateStep();

};
