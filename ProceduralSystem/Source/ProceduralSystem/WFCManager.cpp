// Fill out your copyright notice in the Description page of Project Settings.


#include "WFCManager.h"
#include "Engine/StaticMeshActor.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"

// Sets default values
AWFCManager::AWFCManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bIsGenerating = false;

}

// Called when the game starts or when spawned
void AWFCManager::BeginPlay()
{
	Super::BeginPlay();
	
	LoadTiles();
    
	if (bAutoGenerate)
	{
		FTimerHandle DelayHandle;
		GetWorld()->GetTimerManager().SetTimer(DelayHandle, this, &AWFCManager::StartGeneration, 0.5f, false);
	}
	
}

// Called every frame
void AWFCManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (bShowDebugBoxes)
	{
		for (const FWFCGridCell& Cell : Grid)
		{
			if (!Cell.bIsCollapsed)
			{
				FColor DebugColor = Cell.OptionsCount > 3 ? FColor::Green : FColor::Red;
				DrawDebugBox(GetWorld(), Cell.WorldLocation, FVector(50, 50, 50), DebugColor, false, -1, 0, 2);
			}
		}
	}

}

int32 AWFCManager::GetGridIndex(int32 X, int32 Y) const
{
    return (Y * GridSizeX) + X;
}

bool AWFCManager::GetCellAt(int32 X, int32 Y, FWFCGridCell& OutCell) const
{
    if (X < 0 || X >= GridSizeX || Y < 0 || Y >= GridSizeY)
    {
        return false;
    }
    
    int32 Index = GetGridIndex(X, Y);
    if (!Grid.IsValidIndex(Index))
    {
        return false;
    }
    
    OutCell = Grid[Index];
    return true;
}

void AWFCManager::SetCellAt(int32 X, int32 Y, const FWFCGridCell& NewCell)
{
    int32 Index = GetGridIndex(X, Y);
    if (Grid.IsValidIndex(Index))
    {
        Grid[Index] = NewCell;
    }
}

bool AWFCManager::CanConnect(const FWFCTileData& Tile1, const FWFCTileData& Tile2, int32 Direction) const
{
    switch (Direction)
    {
        case 0: return Tile1.SocketNorth == Tile2.SocketSouth;
        case 1: return Tile1.SocketEast == Tile2.SocketWest;
        case 2: return Tile1.SocketSouth == Tile2.SocketNorth;
        case 3: return Tile1.SocketWest == Tile2.SocketEast;
        default: return false;
    }
}

void AWFCManager::LoadTiles()
{
    AllTiles.Empty();
    
    if (!TileDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("WFCManager: TileDataTable is not set!"));
        return;
    }
    
    TArray<FWFCTileData*> Rows;
    TileDataTable->GetAllRows<FWFCTileData>(TEXT("LoadTiles"), Rows);
    
    for (FWFCTileData* Row : Rows)
    {
        if (Row)
        {
            AllTiles.Add(*Row);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("WFCManager: Loaded %d tiles"), AllTiles.Num());
}

void AWFCManager::InitializeGrid()
{
    Grid.Empty();
    
    for (int32 Y = 0; Y < GridSizeY; Y++)
    {
        for (int32 X = 0; X < GridSizeX; X++)
        {
            FWFCGridCell NewCell;
            NewCell.bIsCollapsed = false;
            NewCell.PossibleTiles = AllTiles;
            NewCell.OptionsCount = AllTiles.Num();
            NewCell.GridX = X;
            NewCell.GridY = Y;
            NewCell.WorldLocation = GetActorLocation() + FVector(X * TileSize, Y * TileSize, 0);
            NewCell.SpawnedActor = nullptr;
            
            Grid.Add(NewCell);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("WFCManager: Grid initialized with %d cells"), Grid.Num());
}

TArray<FWFCTileData> AWFCManager::GetValidTiles(int32 X, int32 Y)
{
    TArray<FWFCTileData> ValidTiles = AllTiles;
    
    FWFCGridCell NorthCell;
    if (GetCellAt(X, Y - 1, NorthCell) && NorthCell.bIsCollapsed)
    {
        ValidTiles = ValidTiles.FilterByPredicate([&](const FWFCTileData& Tile)
        {
            return CanConnect(Tile, NorthCell.ChosenTile, 0);
        });
    }
    
    FWFCGridCell EastCell;
    if (GetCellAt(X + 1, Y, EastCell) && EastCell.bIsCollapsed)
    {
        ValidTiles = ValidTiles.FilterByPredicate([&](const FWFCTileData& Tile)
        {
            return CanConnect(Tile, EastCell.ChosenTile, 1);
        });
    }
    
    FWFCGridCell SouthCell;
    if (GetCellAt(X, Y + 1, SouthCell) && SouthCell.bIsCollapsed)
    {
        ValidTiles = ValidTiles.FilterByPredicate([&](const FWFCTileData& Tile)
        {
            return CanConnect(Tile, SouthCell.ChosenTile, 2);
        });
    }
    
    FWFCGridCell WestCell;
    if (GetCellAt(X - 1, Y, WestCell) && WestCell.bIsCollapsed)
    {
        ValidTiles = ValidTiles.FilterByPredicate([&](const FWFCTileData& Tile)
        {
            return CanConnect(Tile, WestCell.ChosenTile, 3);
        });
    }
    
    return ValidTiles;
}

bool AWFCManager::FindCellWithFewestOptions(int32& OutX, int32& OutY)
{
    int32 MinOptions = MAX_int32;
    TArray<FIntPoint> Candidates;
    
    for (const FWFCGridCell& Cell : Grid)
    {
        if (!Cell.bIsCollapsed && Cell.OptionsCount > 0)
        {
            if (Cell.OptionsCount < MinOptions)
            {
                MinOptions = Cell.OptionsCount;
                Candidates.Empty();
                Candidates.Add(FIntPoint(Cell.GridX, Cell.GridY));
            }
            else if (Cell.OptionsCount == MinOptions)
            {
                Candidates.Add(FIntPoint(Cell.GridX, Cell.GridY));
            }
        }
    }
    
    if (Candidates.Num() > 0)
    {
        FIntPoint Chosen = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
        OutX = Chosen.X;
        OutY = Chosen.Y;
        return true;
    }
    
    return false;
}

void AWFCManager::CollapseCell(int32 X, int32 Y)
{
    FWFCGridCell Cell;
    if (!GetCellAt(X, Y, Cell))
    {
        return;
    }
    
    if (Cell.PossibleTiles.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("WFCManager: No valid tiles for cell (%d, %d)!"), X, Y);
        return;
    }
    
    int32 RandomIndex = FMath::RandRange(0, Cell.PossibleTiles.Num() - 1);
    FWFCTileData ChosenTile = Cell.PossibleTiles[RandomIndex];
    
    Cell.bIsCollapsed = true;
    Cell.ChosenTile = ChosenTile;
    Cell.OptionsCount = 0;
    Cell.PossibleTiles.Empty();
    
    SetCellAt(X, Y, Cell);
    SpawnTile(X, Y, ChosenTile);
}

void AWFCManager::PropagateConstraints()
{
    bool bChanged = true;
    int32 Iterations = 0;
    const int32 MaxIterations = 1000;
    
    while (bChanged && Iterations < MaxIterations)
    {
        bChanged = false;
        Iterations++;
        
        for (int32 i = 0; i < Grid.Num(); i++)
        {
            FWFCGridCell& Cell = Grid[i];
            
            if (!Cell.bIsCollapsed)
            {
                TArray<FWFCTileData> ValidTiles = GetValidTiles(Cell.GridX, Cell.GridY);
                
                if (ValidTiles.Num() < Cell.PossibleTiles.Num())
                {
                    Cell.PossibleTiles = ValidTiles;
                    Cell.OptionsCount = ValidTiles.Num();
                    bChanged = true;
                }
            }
        }
    }
    
    if (Iterations >= MaxIterations)
    {
        UE_LOG(LogTemp, Warning, TEXT("WFCManager: PropagateConstraints hit max iterations"));
    }
}

void AWFCManager::SpawnTile(int32 X, int32 Y, const FWFCTileData& TileData)
{
    if (!TileData.TileMesh)
    {
        return;
    }
    
    FWFCGridCell Cell;
    if (!GetCellAt(X, Y, Cell))
    {
        return;
    }
    
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    
    AStaticMeshActor* TileActor = GetWorld()->SpawnActor<AStaticMeshActor>(
        AStaticMeshActor::StaticClass(),
        Cell.WorldLocation,
        FRotator::ZeroRotator,
        SpawnParams
    );
    
    if (TileActor)
    {
        TileActor->GetStaticMeshComponent()->SetStaticMesh(TileData.TileMesh);
        TileActor->SetActorLabel(FString::Printf(TEXT("Tile_%d_%d_%s"), X, Y, *TileData.TileName));
        
        Cell.SpawnedActor = TileActor;
        SetCellAt(X, Y, Cell);
    }
}

void AWFCManager::GenerateStep()
{
    int32 X, Y;
    if (!FindCellWithFewestOptions(X, Y))
    {
        UE_LOG(LogTemp, Log, TEXT("WFCManager: Generation Complete!"));
        StopGeneration();
        return;
    }
    
    CollapseCell(X, Y);
    PropagateConstraints();
}

void AWFCManager::StartGeneration()
{
    UE_LOG(LogTemp, Log, TEXT("WFCManager: Starting generation..."));
    
    ClearGrid();
    InitializeGrid();
    
    bIsGenerating = true;
    
    GetWorld()->GetTimerManager().SetTimer(
        GenerationTimerHandle,
        this,
        &AWFCManager::GenerateStep,
        GenerationSpeed,
        true
    );
}

void AWFCManager::StopGeneration()
{
    bIsGenerating = false;
    GetWorld()->GetTimerManager().ClearTimer(GenerationTimerHandle);
    UE_LOG(LogTemp, Log, TEXT("WFCManager: Generation stopped"));
}

void AWFCManager::ClearGrid()
{
    for (FWFCGridCell& Cell : Grid)
    {
        if (Cell.SpawnedActor)
        {
            Cell.SpawnedActor->Destroy();
        }
    }
    Grid.Empty();
}

