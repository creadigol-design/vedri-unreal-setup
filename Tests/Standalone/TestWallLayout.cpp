// Copyright vedri studio Ltd. All Rights Reserved.

#include "TestHarness.h"

#include "Core/VPCoreWallLayout.h"

#include <cmath>
#include <utility>
#include <vector>

using namespace Vedri::Core;

namespace
{
	StageVec3 Sub(const StageVec3& A, const StageVec3& B)
	{
		return StageVec3{A.Right - B.Right, A.Up - B.Up, A.TowardStage - B.TowardStage};
	}

	/** Right-handed cross product in (Right, Up, TowardStage). */
	StageVec3 Cross(const StageVec3& A, const StageVec3& B)
	{
		return StageVec3{A.Up * B.TowardStage - A.TowardStage * B.Up, A.TowardStage * B.Right - A.Right * B.TowardStage, A.Right * B.Up - A.Up * B.Right};
	}

	StageVec3 TriangleNormal(const SurfaceMesh& Mesh, size_t Triangle)
	{
		const StageVec3& A = Mesh.Positions[Mesh.Triangles[Triangle * 3]];
		const StageVec3& B = Mesh.Positions[Mesh.Triangles[Triangle * 3 + 1]];
		const StageVec3& C = Mesh.Positions[Mesh.Triangles[Triangle * 3 + 2]];
		return Cross(Sub(B, A), Sub(C, A));
	}
}

VEDRI_TEST(Wall_DefaultIsPlanned8x3Wall)
{
	const WallSpec Spec;
	const WallLayout Layout = ComputeWallLayout(Spec);
	ASSERT_TRUE(Layout.IsValid());
	EXPECT_TRUE(Layout.Issues.empty());
	EXPECT_EQ(Layout.Wall.TilesX, 16u);
	EXPECT_EQ(Layout.Wall.TilesY, 6u);
	EXPECT_EQ(Layout.Wall.PixelsPerTileX, 176u);
	EXPECT_EQ(Layout.Wall.PixelsX, 2816u);
	EXPECT_EQ(Layout.Wall.PixelsY, 1056u);
	EXPECT_NEAR(Layout.Wall.ActualWidthM, 8.0, 1e-9);
	EXPECT_NEAR(Layout.Wall.ActualHeightM, 3.0, 1e-9);
	EXPECT_NEAR(Layout.Wall.EffectivePitchXMm, 500.0 / 176.0, 1e-9);

	ASSERT_TRUE(Layout.Nodes.size() == 2);
	ASSERT_TRUE(Layout.Viewports.size() == 2);
	EXPECT_EQ(Layout.Nodes[0].Id, "node_0");
	EXPECT_EQ(Layout.Nodes[1].Id, "node_1");
	for (uint32_t Index = 0; Index < 2; ++Index)
	{
		const ViewportLayout& Viewport = Layout.Viewports[Index];
		EXPECT_EQ(Viewport.NodeIndex, Index);
		EXPECT_EQ(Viewport.Width, 1408u);
		EXPECT_EQ(Viewport.Height, 1056u);
		EXPECT_EQ(Viewport.CanvasX, Index * 1408u);
		EXPECT_EQ(Viewport.WindowX, 0u);
		EXPECT_EQ(Viewport.TileCountX, 8u);
		EXPECT_NEAR(Viewport.U0M, 4.0 * Index, 1e-9);
		EXPECT_NEAR(Viewport.U1M, 4.0 * (Index + 1), 1e-9);
		EXPECT_EQ(Layout.Nodes[Index].WindowWidth, 1408u);
		EXPECT_EQ(Layout.Nodes[Index].WindowHeight, 1056u);
	}
	EXPECT_EQ(Layout.Viewports[0].Id, "wall_0");
}

VEDRI_TEST(Wall_UnevenSplitStaysTileAligned)
{
	WallSpec Spec;
	Spec.RenderNodeCount = 3;
	const WallLayout Layout = ComputeWallLayout(Spec);
	ASSERT_TRUE(Layout.IsValid());
	ASSERT_TRUE(Layout.Viewports.size() == 3);
	EXPECT_EQ(Layout.Viewports[0].TileCountX, 6u);
	EXPECT_EQ(Layout.Viewports[1].TileCountX, 5u);
	EXPECT_EQ(Layout.Viewports[2].TileCountX, 5u);
	uint32_t Covered = 0;
	for (const ViewportLayout& Viewport : Layout.Viewports)
	{
		EXPECT_EQ(Viewport.CanvasX, Covered);
		EXPECT_EQ(Viewport.Width % Layout.Wall.PixelsPerTileX, 0u);
		Covered += Viewport.Width;
	}
	EXPECT_EQ(Covered, Layout.Wall.PixelsX);
	const ValidationIssue* Uneven = FindIssue(Layout.Issues, "wall.unevenNodeSplit");
	ASSERT_TRUE(Uneven != nullptr);
	EXPECT_EQ(Uneven->Severity, ESeverity::Info);
	EXPECT_EQ(Uneven->Args.size(), size_t(2));
	EXPECT_EQ(Uneven->Args[0], "1056");
	EXPECT_EQ(Uneven->Args[1], "880");
}

VEDRI_TEST(Wall_SizeNotWholeTilesWarns)
{
	WallSpec Spec;
	Spec.WidthM = 8.2;
	const WallLayout Layout = ComputeWallLayout(Spec);
	EXPECT_TRUE(Layout.IsValid());
	EXPECT_EQ(Layout.Wall.TilesX, 16u);
	const ValidationIssue* Issue = FindIssue(Layout.Issues, "wall.sizeNotWholeTiles");
	ASSERT_TRUE(Issue != nullptr);
	EXPECT_EQ(Issue->Severity, ESeverity::Warning);
	EXPECT_EQ(Issue->Field, "widthM");
	EXPECT_TRUE(Issue->Args == (std::vector<std::string>{"8.2", "16", "8"}));
}

VEDRI_TEST(Wall_RoundedPublishedPitchesResolveToTheRightPixelCount)
{
	// Published pitch -> real pixel count of a 500 mm tile; none of these should warn.
	const std::vector<std::pair<double, uint32_t>> Panels = {{2.84, 176u}, {2.6, 192u}, {2.5, 200u}, {3.9, 128u}, {2.9, 172u}};
	for (const auto& [Pitch, Expected] : Panels)
	{
		WallSpec Spec;
		Spec.Panels.PixelPitchMm = Pitch;
		const WallLayout Layout = ComputeWallLayout(Spec);
		EXPECT_EQ(Layout.Wall.PixelsPerTileX, Expected);
		EXPECT_TRUE(FindIssue(Layout.Issues, "wall.pitchNotWholePixelsPerTile") == nullptr);
	}
}

VEDRI_TEST(Wall_AmbiguousOrUnfittablePitchWarns)
{
	// 500 / 1.95 = 256.41: both 256 and 257 px are consistent with "1.95", so the pitch cannot decide.
	WallSpec Spec;
	Spec.Panels.PixelPitchMm = 1.95;
	WallLayout Layout = ComputeWallLayout(Spec);
	EXPECT_TRUE(Layout.IsValid());
	EXPECT_EQ(Layout.Wall.PixelsPerTileX, 256u);
	const ValidationIssue* Issue = FindIssue(Layout.Issues, "wall.pitchNotWholePixelsPerTile");
	ASSERT_TRUE(Issue != nullptr);
	EXPECT_EQ(Issue->Severity, ESeverity::Warning);
	EXPECT_EQ(Issue->Field, "panels.pixelsPerTileX");
	EXPECT_EQ(Issue->Args[2], "256.41");

	// 500 / 15 = 33.33: no whole count gives 15 mm within tolerance.
	Spec.Panels.PixelPitchMm = 15.0;
	EXPECT_TRUE(FindIssue(ComputeWallLayout(Spec).Issues, "wall.pitchNotWholePixelsPerTile") != nullptr);

	// The spec-sheet pixel count overrides the pitch and silences the warning.
	Spec.Panels.PixelPitchMm = 1.95;
	Spec.Panels.PixelsPerTileX = 256;
	Spec.Panels.PixelsPerTileY = 256;
	Layout = ComputeWallLayout(Spec);
	EXPECT_TRUE(FindIssue(Layout.Issues, "wall.pitchNotWholePixelsPerTile") == nullptr);
	EXPECT_EQ(Layout.Wall.PixelsX, 16u * 256u);
}

VEDRI_TEST(Wall_NonSquareTiles)
{
	WallSpec Spec;
	Spec.Panels.TileHeightMm = 1000.0;
	const WallLayout Layout = ComputeWallLayout(Spec);
	ASSERT_TRUE(Layout.IsValid());
	EXPECT_EQ(Layout.Wall.TilesY, 3u);
	EXPECT_EQ(Layout.Wall.PixelsPerTileY, 352u);
	EXPECT_EQ(Layout.Wall.PixelsY, 1056u);
}

VEDRI_TEST(Wall_InvalidInputsAreErrors)
{
	WallSpec Spec;
	Spec.WidthM = 0.0;
	EXPECT_TRUE(FindIssue(ComputeWallLayout(Spec).Issues, "wall.dimensionOutOfRange") != nullptr);

	Spec = WallSpec();
	Spec.Panels.PixelPitchMm = -1.0;
	WallLayout Layout = ComputeWallLayout(Spec);
	EXPECT_TRUE(FindIssue(Layout.Issues, "wall.pitchOutOfRange") != nullptr);
	EXPECT_TRUE(Layout.Viewports.empty());

	Spec = WallSpec();
	Spec.RenderNodeCount = 0;
	EXPECT_TRUE(FindIssue(ComputeWallLayout(Spec).Issues, "wall.nodeCountOutOfRange") != nullptr);

	Spec = WallSpec();
	Spec.WidthM = 1.0;
	Spec.RenderNodeCount = 3;
	Layout = ComputeWallLayout(Spec);
	EXPECT_FALSE(Layout.IsValid());
	EXPECT_TRUE(FindIssue(Layout.Issues, "wall.moreNodesThanTileColumns") != nullptr);
	EXPECT_TRUE(Layout.Nodes.empty());

	Spec = WallSpec();
	Spec.WidthM = 0.2;
	EXPECT_TRUE(FindIssue(ComputeWallLayout(Spec).Issues, "wall.smallerThanOneTile") != nullptr);

	Spec = WallSpec();
	Spec.CurvatureRadiusM = 1.0; // 8 m of wall on a 1 m radius wraps more than a full circle
	EXPECT_TRUE(FindIssue(ComputeWallLayout(Spec).Issues, "wall.curvatureTooTight") != nullptr);
}

VEDRI_TEST(Wall_CeilingGoesOnLastNodeByDefault)
{
	WallSpec Spec;
	Spec.Ceiling.bEnabled = true;
	const WallLayout Layout = ComputeWallLayout(Spec);
	ASSERT_TRUE(Layout.IsValid());
	ASSERT_TRUE(Layout.bHasCeiling);
	ASSERT_TRUE(Layout.Viewports.size() == 3);
	const ViewportLayout& Ceiling = Layout.Viewports[2];
	EXPECT_EQ(Ceiling.Id, "ceiling");
	EXPECT_EQ(Ceiling.Surface, ESurface::Ceiling);
	EXPECT_EQ(Ceiling.NodeIndex, 1u);
	EXPECT_EQ(Ceiling.WindowX, 1408u);
	EXPECT_EQ(Ceiling.Width, 2816u);
	EXPECT_EQ(Ceiling.Height, 1056u);
	EXPECT_EQ(Layout.Nodes[1].WindowWidth, 1408u + 2816u);
	EXPECT_EQ(Layout.Nodes[1].ViewportIndices.size(), size_t(2));
	EXPECT_EQ(Layout.Nodes[0].WindowWidth, 1408u);

	WallSpec OnFirst = Spec;
	OnFirst.Ceiling.NodeIndex = 0;
	EXPECT_EQ(ComputeWallLayout(OnFirst).Viewports[2].NodeIndex, 0u);

	WallSpec OutOfRange = Spec;
	OutOfRange.Ceiling.NodeIndex = 5;
	EXPECT_TRUE(FindIssue(ComputeWallLayout(OutOfRange).Issues, "wall.ceilingNodeOutOfRange") != nullptr);
}

VEDRI_TEST(Wall_HugeNodeWindowWarns)
{
	WallSpec Spec;
	Spec.WidthM = 50.0;
	Spec.RenderNodeCount = 1;
	const WallLayout Layout = ComputeWallLayout(Spec);
	EXPECT_TRUE(Layout.IsValid());
	EXPECT_TRUE(FindIssue(Layout.Issues, "wall.nodeWindowTooLarge") != nullptr);
}

VEDRI_TEST(Wall_FlatGeometry)
{
	WallSpec Spec;
	Spec.BottomHeightM = 0.5;
	const WallLayout Layout = ComputeWallLayout(Spec);
	const StageVec3 BottomLeft = WallSurfacePoint(Spec, Layout.Wall, 0.0, 0.0);
	const StageVec3 TopRight = WallSurfacePoint(Spec, Layout.Wall, 8.0, 3.0);
	EXPECT_NEAR(BottomLeft.Right, -4.0, 1e-9);
	EXPECT_NEAR(BottomLeft.Up, 0.5, 1e-9);
	EXPECT_NEAR(BottomLeft.TowardStage, 0.0, 1e-9);
	EXPECT_NEAR(TopRight.Right, 4.0, 1e-9);
	EXPECT_NEAR(TopRight.Up, 3.5, 1e-9);
}

VEDRI_TEST(Wall_CurvedGeometryPreservesArcLength)
{
	WallSpec Spec;
	Spec.CurvatureRadiusM = 10.0;
	const WallLayout Layout = ComputeWallLayout(Spec);
	ASSERT_TRUE(Layout.IsValid());
	const StageVec3 Left = WallSurfacePoint(Spec, Layout.Wall, 0.0, 0.0);
	const StageVec3 Centre = WallSurfacePoint(Spec, Layout.Wall, 4.0, 0.0);
	EXPECT_NEAR(Left.Right, 10.0 * std::sin(-0.4), 1e-9);
	EXPECT_NEAR(Left.TowardStage, 10.0 * (1.0 - std::cos(0.4)), 1e-9);
	EXPECT_TRUE(Left.TowardStage > 0.0); // edges curve toward the stage
	EXPECT_NEAR(Centre.Right, 0.0, 1e-9);
	EXPECT_NEAR(Centre.TowardStage, 0.0, 1e-9);

	double Length = 0.0;
	StageVec3 Previous = Left;
	for (int Step = 1; Step <= 1000; ++Step)
	{
		const StageVec3 Next = WallSurfacePoint(Spec, Layout.Wall, 8.0 * Step / 1000.0, 0.0);
		const StageVec3 Delta = Sub(Next, Previous);
		Length += std::sqrt(Delta.Right * Delta.Right + Delta.TowardStage * Delta.TowardStage);
		Previous = Next;
	}
	EXPECT_NEAR(Length, 8.0, 1e-4);
}

VEDRI_TEST(Wall_FlatMeshCornersAndUvs)
{
	const WallSpec Spec;
	const WallLayout Layout = ComputeWallLayout(Spec);
	ASSERT_TRUE(Layout.IsValid());
	const ViewportLayout& Viewport = Layout.Viewports[0];
	EXPECT_EQ(RecommendedMeshColumns(Spec, Viewport), 1u);
	const SurfaceMesh Mesh = BuildViewportMesh(Spec, Layout, Viewport, 1, 1);
	ASSERT_TRUE(Mesh.Positions.size() == 4);
	EXPECT_EQ(Mesh.Triangles.size(), size_t(6));
	// Vertex 0 is the top-left of the image: left edge of the wall, top of the wall.
	EXPECT_NEAR(Mesh.Positions[0].Right, -4.0, 1e-9);
	EXPECT_NEAR(Mesh.Positions[0].Up, 3.0, 1e-9);
	EXPECT_NEAR(Mesh.UVs[0].X, 0.0, 1e-12);
	EXPECT_NEAR(Mesh.UVs[0].Y, 0.0, 1e-12);
	// Vertex 3 is the bottom-right: the seam with node_1 at Right = 0, floor level.
	EXPECT_NEAR(Mesh.Positions[3].Right, 0.0, 1e-9);
	EXPECT_NEAR(Mesh.Positions[3].Up, 0.0, 1e-9);
	EXPECT_NEAR(Mesh.UVs[3].X, 1.0, 1e-12);
	EXPECT_NEAR(Mesh.UVs[3].Y, 1.0, 1e-12);
	for (size_t Triangle = 0; Triangle < Mesh.Triangles.size() / 3; ++Triangle)
	{
		EXPECT_TRUE(TriangleNormal(Mesh, Triangle).TowardStage > 0.0);
	}
}

VEDRI_TEST(Wall_CurvedMeshFacesTheCamera)
{
	WallSpec Spec;
	Spec.CurvatureRadiusM = 6.0;
	const WallLayout Layout = ComputeWallLayout(Spec);
	ASSERT_TRUE(Layout.IsValid());
	for (const ViewportLayout& Viewport : Layout.Viewports)
	{
		const uint32_t Columns = RecommendedMeshColumns(Spec, Viewport);
		EXPECT_EQ(Columns, 32u);
		const SurfaceMesh Mesh = BuildViewportMesh(Spec, Layout, Viewport, Columns);
		EXPECT_EQ(Mesh.Positions.size(), size_t((Columns + 1) * 2));
		const StageVec3 CentreOfCurvature{0.0, 1.5, Spec.CurvatureRadiusM};
		for (size_t Triangle = 0; Triangle < Mesh.Triangles.size() / 3; ++Triangle)
		{
			const StageVec3 Normal = TriangleNormal(Mesh, Triangle);
			const StageVec3 ToCentre = Sub(CentreOfCurvature, Mesh.Positions[Mesh.Triangles[Triangle * 3]]);
			EXPECT_TRUE(Normal.Right * ToCentre.Right + Normal.Up * ToCentre.Up + Normal.TowardStage * ToCentre.TowardStage > 0.0);
		}
	}
}

VEDRI_TEST(Wall_CeilingMeshFacesDownAndReadsUpright)
{
	WallSpec Spec;
	Spec.Ceiling.bEnabled = true;
	Spec.Ceiling.WidthM = 4.0;
	Spec.Ceiling.DepthM = 2.0;
	const WallLayout Layout = ComputeWallLayout(Spec);
	ASSERT_TRUE(Layout.IsValid());
	const ViewportLayout& Viewport = Layout.Viewports.back();
	ASSERT_TRUE(Viewport.Surface == ESurface::Ceiling);
	const SurfaceMesh Mesh = BuildViewportMesh(Spec, Layout, Viewport, 2, 2);
	EXPECT_EQ(Mesh.Positions.size(), size_t(9));
	// Top-left of the image: left edge, far from the wall; at the top of the 3 m wall.
	EXPECT_NEAR(Mesh.Positions[0].Right, -2.0, 1e-9);
	EXPECT_NEAR(Mesh.Positions[0].TowardStage, 2.0, 1e-9);
	EXPECT_NEAR(Mesh.Positions[0].Up, 3.0, 1e-9);
	// Bottom-right: right edge, at the wall.
	EXPECT_NEAR(Mesh.Positions[8].Right, 2.0, 1e-9);
	EXPECT_NEAR(Mesh.Positions[8].TowardStage, 0.0, 1e-9);
	for (size_t Triangle = 0; Triangle < Mesh.Triangles.size() / 3; ++Triangle)
	{
		EXPECT_TRUE(TriangleNormal(Mesh, Triangle).Up < 0.0);
	}

	WallSpec Raised = Spec;
	Raised.Ceiling.HeightM = 4.5;
	EXPECT_NEAR(ResolveCeilingHeightM(Raised, Layout.Wall), 4.5, 1e-9);
}
