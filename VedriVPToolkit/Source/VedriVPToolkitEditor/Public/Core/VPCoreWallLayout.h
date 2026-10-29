// Copyright vedri studio Ltd. All Rights Reserved.

#pragma once

// LED wall maths: tile grid and pixel resolution from physical size and pitch, the split of the wall
// across render nodes, and the surface geometry each viewport maps onto.
//
// Geometry is expressed in an engine-neutral, right-handed, metre-based "stage frame":
//   Right       - along the wall, left to right as seen from the stage
//   Up          - vertical
//   TowardStage - wall normal, pointing from the wall toward the camera
// Origin: on the floor, directly below the horizontal centre of the wall surface.
// The editor layer converts to Unreal's left-handed, centimetre frame when it builds the nDisplay
// config; that conversion is NOT done here so it can be verified once against nDisplay on the PC.

#include "Core/VPCoreValidation.h"

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace Vedri::Core
{
	/** LED panel (tile) model. Pixels per tile come from the spec sheet when known, else from the pitch. */
	struct PanelSpec
	{
		double PixelPitchMm = 2.84;
		double TileWidthMm = 500.0;
		double TileHeightMm = 500.0;
		/** 0 = derive from TileWidthMm / PixelPitchMm. Set from the panel spec sheet when they disagree. */
		uint32_t PixelsPerTileX = 0;
		/** 0 = derive from TileHeightMm / PixelPitchMm. */
		uint32_t PixelsPerTileY = 0;

		bool operator==(const PanelSpec& Other) const = default;
	};

	/** Optional flat ceiling. Width runs along the wall (Right), depth runs from the wall toward the stage. */
	struct CeilingSpec
	{
		bool bEnabled = false;
		double WidthM = 8.0;
		double DepthM = 3.0;
		PanelSpec Panels;
		/** Height of the ceiling's LED surface above the floor. 0 = level with the top of the wall. */
		double HeightM = 0.0;
		/** Render node that drives the ceiling. -1 = the last node. */
		int32_t NodeIndex = -1;

		bool operator==(const CeilingSpec& Other) const = default;
	};

	/**
	 * Wall specs as entered in the wizard. Defaults are vedri studio's planned 8 m x 3 m wall.
	 * TODO(confirm-spec): the 2.84 mm / 500 mm panel (176 px per tile) is a placeholder until the panel
	 * model is chosen.
	 */
	struct WallSpec
	{
		double WidthM = 8.0;
		double HeightM = 3.0;
		PanelSpec Panels;
		uint32_t RenderNodeCount = 2;
		/** 0 = flat. Otherwise the radius of a concave (toward the stage) horizontal curve. */
		double CurvatureRadiusM = 0.0;
		/** Height of the wall's bottom LED row above the floor (e.g. on a riser). */
		double BottomHeightM = 0.0;
		CeilingSpec Ceiling;

		bool operator==(const WallSpec& Other) const = default;
	};

	/** Derived tile and pixel grid of one surface. */
	struct PanelGrid
	{
		uint32_t TilesX = 0;
		uint32_t TilesY = 0;
		uint32_t PixelsPerTileX = 0;
		uint32_t PixelsPerTileY = 0;
		uint32_t PixelsX = 0;
		uint32_t PixelsY = 0;
		/** Physical size built from whole tiles (may differ slightly from what was entered). */
		double ActualWidthM = 0.0;
		double ActualHeightM = 0.0;
		/** Tile size divided by pixels per tile: the true pitch once pixels per tile are integers. */
		double EffectivePitchXMm = 0.0;
		double EffectivePitchYMm = 0.0;
	};

	enum class ESurface : uint8_t
	{
		Wall,
		Ceiling
	};

	struct ViewportLayout
	{
		std::string Id;
		ESurface Surface = ESurface::Wall;
		uint32_t NodeIndex = 0;
		/** Placement inside the node's render window, in pixels. */
		uint32_t WindowX = 0;
		uint32_t WindowY = 0;
		uint32_t Width = 0;
		uint32_t Height = 0;
		/** Top-left of the region this viewport covers in the surface's own pixel canvas. */
		uint32_t CanvasX = 0;
		uint32_t CanvasY = 0;
		/** The same region in whole tile columns. */
		uint32_t FirstTileX = 0;
		uint32_t TileCountX = 0;
		/**
		 * The same region in metres on the surface. Wall: U is arc length from the left edge, V is height
		 * above the wall's bottom edge. Ceiling: U runs along Right from its left edge, V runs from the
		 * wall edge toward the stage.
		 */
		double U0M = 0.0;
		double U1M = 0.0;
		double V0M = 0.0;
		double V1M = 0.0;
	};

	struct NodeLayout
	{
		std::string Id;
		uint32_t WindowWidth = 0;
		uint32_t WindowHeight = 0;
		/** Indices into WallLayout::Viewports. */
		std::vector<size_t> ViewportIndices;
	};

	struct WallLayout
	{
		PanelGrid Wall;
		bool bHasCeiling = false;
		PanelGrid Ceiling;
		std::vector<NodeLayout> Nodes;
		std::vector<ViewportLayout> Viewports;
		IssueList Issues;

		bool IsValid() const { return !HasErrors(Issues); }
	};

	namespace WallDetail
	{
		inline constexpr double TwoPi = 6.283185307179586;
		/** Render targets above this size fail on current GPUs; warn well before the hard limit. */
		inline constexpr uint32_t MaxWindowDimension = 16384;

		inline bool ValidatePanels(const PanelSpec& Panels, const std::string& Prefix, IssueList& Issues)
		{
			bool bOk = true;
			if (!(Panels.PixelPitchMm > 0.0 && Panels.PixelPitchMm <= 50.0))
			{
				AddIssue(Issues, ESeverity::Error, "wall.pitchOutOfRange", Prefix + "pixelPitchMm", {FormatNumber(Panels.PixelPitchMm)});
				bOk = false;
			}
			if (!(Panels.TileWidthMm > 0.0 && Panels.TileWidthMm <= 2000.0))
			{
				AddIssue(Issues, ESeverity::Error, "wall.tileSizeOutOfRange", Prefix + "tileWidthMm", {FormatNumber(Panels.TileWidthMm)});
				bOk = false;
			}
			if (!(Panels.TileHeightMm > 0.0 && Panels.TileHeightMm <= 2000.0))
			{
				AddIssue(Issues, ESeverity::Error, "wall.tileSizeOutOfRange", Prefix + "tileHeightMm", {FormatNumber(Panels.TileHeightMm)});
				bOk = false;
			}
			if (Panels.PixelsPerTileX > 8192 || Panels.PixelsPerTileY > 8192)
			{
				AddIssue(Issues, ESeverity::Error, "wall.pixelsPerTileOutOfRange", Prefix + "pixelsPerTile");
				bOk = false;
			}
			return bOk;
		}

		inline uint32_t ResolvePixelsPerTile(uint32_t Explicit, double TileMm, double PitchMm, const std::string& Field, IssueList& Issues)
		{
			if (Explicit > 0)
			{
				return Explicit;
			}
			const double Exact = TileMm / PitchMm;
			const double Rounded = std::round(Exact);
			if (Rounded < 1.0 || Rounded > 8192.0)
			{
				AddIssue(Issues, ESeverity::Error, "wall.pixelsPerTileOutOfRange", Field, {FormatNumber(Exact, 2)});
				return 0;
			}
			// Real panels have a whole number of pixels per tile, and published pitches are rounded
			// (500 mm / 176 px = 2.8409 mm is sold as "2.84"). A count "fits" when the pitch it implies is
			// within 0.25% of the entered pitch. Warn when the nearest count does not fit, or when the
			// next count fits too (then the pitch alone cannot decide): the spec-sheet count is needed.
			const auto Fits = [TileMm, PitchMm](double Count) { return Count >= 1.0 && std::fabs(TileMm / Count - PitchMm) / PitchMm <= 0.0025; };
			const double SecondNearest = Exact >= Rounded ? Rounded + 1.0 : Rounded - 1.0;
			if (!Fits(Rounded) || Fits(SecondNearest))
			{
				AddIssue(Issues, ESeverity::Warning, "wall.pitchNotWholePixelsPerTile", Field,
					{FormatNumber(TileMm), FormatNumber(PitchMm), FormatNumber(Exact, 2), FormatNumber(Rounded, 0)});
			}
			return static_cast<uint32_t>(Rounded);
		}

		inline uint32_t ResolveTileCount(double SizeM, double TileMm, const std::string& Field, IssueList& Issues)
		{
			const double SizeMm = SizeM * 1000.0;
			const double Rounded = std::round(SizeMm / TileMm);
			if (Rounded < 1.0)
			{
				AddIssue(Issues, ESeverity::Error, "wall.smallerThanOneTile", Field, {FormatNumber(SizeM), FormatNumber(TileMm)});
				return 0;
			}
			const double ActualMm = Rounded * TileMm;
			if (std::fabs(ActualMm - SizeMm) > 1.0)
			{
				AddIssue(Issues, ESeverity::Warning, "wall.sizeNotWholeTiles", Field,
					{FormatNumber(SizeM), FormatNumber(Rounded, 0), FormatNumber(ActualMm / 1000.0)});
			}
			return static_cast<uint32_t>(Rounded);
		}

		/** Field prefixes are relative to the wall spec (e.g. "" or "ceiling."). */
		inline bool ComputeGrid(double WidthM, double HeightM, const PanelSpec& Panels, const std::string& Prefix,
			const char* WidthField, const char* HeightField, PanelGrid& Out, IssueList& Issues)
		{
			if (!ValidatePanels(Panels, Prefix + "panels.", Issues))
			{
				return false;
			}
			bool bOk = true;
			if (!(WidthM > 0.0 && WidthM <= 100.0))
			{
				AddIssue(Issues, ESeverity::Error, "wall.dimensionOutOfRange", Prefix + WidthField, {FormatNumber(WidthM)});
				bOk = false;
			}
			if (!(HeightM > 0.0 && HeightM <= 100.0))
			{
				AddIssue(Issues, ESeverity::Error, "wall.dimensionOutOfRange", Prefix + HeightField, {FormatNumber(HeightM)});
				bOk = false;
			}
			if (!bOk)
			{
				return false;
			}

			PanelGrid Grid;
			Grid.TilesX = ResolveTileCount(WidthM, Panels.TileWidthMm, Prefix + WidthField, Issues);
			Grid.TilesY = ResolveTileCount(HeightM, Panels.TileHeightMm, Prefix + HeightField, Issues);
			Grid.PixelsPerTileX = ResolvePixelsPerTile(Panels.PixelsPerTileX, Panels.TileWidthMm, Panels.PixelPitchMm, Prefix + "panels.pixelsPerTileX", Issues);
			Grid.PixelsPerTileY = ResolvePixelsPerTile(Panels.PixelsPerTileY, Panels.TileHeightMm, Panels.PixelPitchMm, Prefix + "panels.pixelsPerTileY", Issues);
			if (Grid.TilesX == 0 || Grid.TilesY == 0 || Grid.PixelsPerTileX == 0 || Grid.PixelsPerTileY == 0)
			{
				return false;
			}
			Grid.PixelsX = Grid.TilesX * Grid.PixelsPerTileX;
			Grid.PixelsY = Grid.TilesY * Grid.PixelsPerTileY;
			Grid.ActualWidthM = static_cast<double>(Grid.TilesX) * Panels.TileWidthMm / 1000.0;
			Grid.ActualHeightM = static_cast<double>(Grid.TilesY) * Panels.TileHeightMm / 1000.0;
			Grid.EffectivePitchXMm = Panels.TileWidthMm / static_cast<double>(Grid.PixelsPerTileX);
			Grid.EffectivePitchYMm = Panels.TileHeightMm / static_cast<double>(Grid.PixelsPerTileY);
			Out = Grid;
			return true;
		}
	}

	/** Derives the full layout. On any Error issue, Nodes and Viewports are left empty. */
	inline WallLayout ComputeWallLayout(const WallSpec& Spec)
	{
		WallLayout Layout;
		IssueList& Issues = Layout.Issues;

		const bool bWallOk = WallDetail::ComputeGrid(Spec.WidthM, Spec.HeightM, Spec.Panels, "", "widthM", "heightM", Layout.Wall, Issues);

		if (Spec.RenderNodeCount < 1 || Spec.RenderNodeCount > 64)
		{
			AddIssue(Issues, ESeverity::Error, "wall.nodeCountOutOfRange", "renderNodes", {FormatUInt(Spec.RenderNodeCount)});
		}
		if (Spec.CurvatureRadiusM < 0.0)
		{
			AddIssue(Issues, ESeverity::Error, "wall.curvatureNegative", "curvatureRadiusM", {FormatNumber(Spec.CurvatureRadiusM)});
		}
		else if (bWallOk && Spec.CurvatureRadiusM > 0.0 && Layout.Wall.ActualWidthM / Spec.CurvatureRadiusM >= WallDetail::TwoPi)
		{
			AddIssue(Issues, ESeverity::Error, "wall.curvatureTooTight", "curvatureRadiusM",
				{FormatNumber(Spec.CurvatureRadiusM), FormatNumber(Layout.Wall.ActualWidthM)});
		}
		if (Spec.BottomHeightM < 0.0 || Spec.BottomHeightM > 20.0)
		{
			AddIssue(Issues, ESeverity::Error, "wall.bottomHeightOutOfRange", "bottomHeightM", {FormatNumber(Spec.BottomHeightM)});
		}
		if (bWallOk && Spec.RenderNodeCount >= 1 && Layout.Wall.TilesX < Spec.RenderNodeCount)
		{
			AddIssue(Issues, ESeverity::Error, "wall.moreNodesThanTileColumns", "renderNodes",
				{FormatUInt(Spec.RenderNodeCount), FormatUInt(Layout.Wall.TilesX)});
		}

		bool bCeilingOk = true;
		if (Spec.Ceiling.bEnabled)
		{
			bCeilingOk = WallDetail::ComputeGrid(Spec.Ceiling.WidthM, Spec.Ceiling.DepthM, Spec.Ceiling.Panels, "ceiling.", "widthM", "depthM", Layout.Ceiling, Issues);
			if (Spec.Ceiling.HeightM < 0.0 || Spec.Ceiling.HeightM > 30.0)
			{
				AddIssue(Issues, ESeverity::Error, "wall.ceilingHeightOutOfRange", "ceiling.heightM", {FormatNumber(Spec.Ceiling.HeightM)});
			}
			if (Spec.Ceiling.NodeIndex < -1 || (Spec.Ceiling.NodeIndex >= 0 && static_cast<uint32_t>(Spec.Ceiling.NodeIndex) >= Spec.RenderNodeCount))
			{
				AddIssue(Issues, ESeverity::Error, "wall.ceilingNodeOutOfRange", "ceiling.nodeIndex",
					{std::to_string(Spec.Ceiling.NodeIndex), FormatUInt(Spec.RenderNodeCount)});
			}
			Layout.bHasCeiling = bCeilingOk;
		}

		if (!Layout.IsValid() || !bWallOk || !bCeilingOk)
		{
			return Layout;
		}

		const uint32_t NodeCount = Spec.RenderNodeCount;
		Layout.Nodes.resize(NodeCount);
		for (uint32_t NodeIndex = 0; NodeIndex < NodeCount; ++NodeIndex)
		{
			Layout.Nodes[NodeIndex].Id = "node_" + std::to_string(NodeIndex);
		}

		// Wall: whole tile columns, spread as evenly as possible, earlier nodes take the remainder.
		// Tile-aligned splits keep every seam on a panel edge, which is where the LED processor maps too.
		const uint32_t BaseColumns = Layout.Wall.TilesX / NodeCount;
		const uint32_t ExtraColumns = Layout.Wall.TilesX % NodeCount;
		const double TileWidthM = Spec.Panels.TileWidthMm / 1000.0;
		uint32_t NextTile = 0;
		for (uint32_t NodeIndex = 0; NodeIndex < NodeCount; ++NodeIndex)
		{
			ViewportLayout Viewport;
			Viewport.Id = "wall_" + std::to_string(NodeIndex);
			Viewport.Surface = ESurface::Wall;
			Viewport.NodeIndex = NodeIndex;
			Viewport.FirstTileX = NextTile;
			Viewport.TileCountX = BaseColumns + (NodeIndex < ExtraColumns ? 1u : 0u);
			Viewport.CanvasX = Viewport.FirstTileX * Layout.Wall.PixelsPerTileX;
			Viewport.CanvasY = 0;
			Viewport.Width = Viewport.TileCountX * Layout.Wall.PixelsPerTileX;
			Viewport.Height = Layout.Wall.PixelsY;
			Viewport.U0M = static_cast<double>(Viewport.FirstTileX) * TileWidthM;
			Viewport.U1M = static_cast<double>(Viewport.FirstTileX + Viewport.TileCountX) * TileWidthM;
			Viewport.V0M = 0.0;
			Viewport.V1M = Layout.Wall.ActualHeightM;
			NextTile += Viewport.TileCountX;
			Layout.Viewports.push_back(Viewport);
		}
		if (ExtraColumns != 0)
		{
			AddIssue(Issues, ESeverity::Info, "wall.unevenNodeSplit", "renderNodes",
				{FormatUInt((BaseColumns + 1) * Layout.Wall.PixelsPerTileX), FormatUInt(BaseColumns * Layout.Wall.PixelsPerTileX)});
		}

		if (Layout.bHasCeiling)
		{
			ViewportLayout Viewport;
			Viewport.Id = "ceiling";
			Viewport.Surface = ESurface::Ceiling;
			Viewport.NodeIndex = Spec.Ceiling.NodeIndex < 0 ? NodeCount - 1 : static_cast<uint32_t>(Spec.Ceiling.NodeIndex);
			Viewport.FirstTileX = 0;
			Viewport.TileCountX = Layout.Ceiling.TilesX;
			Viewport.Width = Layout.Ceiling.PixelsX;
			Viewport.Height = Layout.Ceiling.PixelsY;
			Viewport.U0M = 0.0;
			Viewport.U1M = Layout.Ceiling.ActualWidthM;
			Viewport.V0M = 0.0;
			Viewport.V1M = Layout.Ceiling.ActualHeightM;
			Layout.Viewports.push_back(Viewport);
		}

		// Pack each node's viewports left to right in its window.
		for (size_t Index = 0; Index < Layout.Viewports.size(); ++Index)
		{
			ViewportLayout& Viewport = Layout.Viewports[Index];
			NodeLayout& Node = Layout.Nodes[Viewport.NodeIndex];
			Viewport.WindowX = Node.WindowWidth;
			Viewport.WindowY = 0;
			Node.WindowWidth += Viewport.Width;
			Node.WindowHeight = Node.WindowHeight > Viewport.Height ? Node.WindowHeight : Viewport.Height;
			Node.ViewportIndices.push_back(Index);
		}
		for (const NodeLayout& Node : Layout.Nodes)
		{
			if (Node.WindowWidth > WallDetail::MaxWindowDimension || Node.WindowHeight > WallDetail::MaxWindowDimension)
			{
				AddIssue(Issues, ESeverity::Warning, "wall.nodeWindowTooLarge", "renderNodes",
					{Node.Id, FormatUInt(Node.WindowWidth), FormatUInt(Node.WindowHeight)});
			}
		}
		return Layout;
	}

	struct StageVec3
	{
		double Right = 0.0;
		double Up = 0.0;
		double TowardStage = 0.0;
	};

	struct StageVec2
	{
		double X = 0.0;
		double Y = 0.0;
	};

	/** Point on the wall at arc length U from its left edge and height V above its bottom edge. */
	inline StageVec3 WallSurfacePoint(const WallSpec& Spec, const PanelGrid& Wall, double UM, double VM)
	{
		const double FromCentre = UM - Wall.ActualWidthM * 0.5;
		StageVec3 Point;
		Point.Up = Spec.BottomHeightM + VM;
		if (Spec.CurvatureRadiusM > 0.0)
		{
			// Concave toward the stage: the centre of curvature is at TowardStage = R.
			const double Angle = FromCentre / Spec.CurvatureRadiusM;
			Point.Right = Spec.CurvatureRadiusM * std::sin(Angle);
			Point.TowardStage = Spec.CurvatureRadiusM * (1.0 - std::cos(Angle));
		}
		else
		{
			Point.Right = FromCentre;
		}
		return Point;
	}

	/** Height of the ceiling's LED surface above the floor. */
	inline double ResolveCeilingHeightM(const WallSpec& Spec, const PanelGrid& Wall)
	{
		return Spec.Ceiling.HeightM > 0.0 ? Spec.Ceiling.HeightM : Spec.BottomHeightM + Wall.ActualHeightM;
	}

	/** Point on the ceiling at U along its width (from its left edge) and V from the wall edge toward the stage. */
	inline StageVec3 CeilingSurfacePoint(const WallSpec& Spec, const WallLayout& Layout, double UM, double VM)
	{
		StageVec3 Point;
		Point.Right = UM - Layout.Ceiling.ActualWidthM * 0.5;
		Point.Up = ResolveCeilingHeightM(Spec, Layout.Wall);
		Point.TowardStage = VM;
		return Point;
	}

	/**
	 * Triangle mesh for one viewport's surface region.
	 * UVs: (0,0) is the top-left of the viewport image, V grows downward (texture convention).
	 * Wall "top" is up. Ceiling "top" is the edge farthest from the wall, so the image reads upright to
	 * someone facing the wall who tilts their head back.
	 * Winding: counter-clockwise as seen by that viewer, i.e. triangle normals face the stage
	 * (+TowardStage for the wall, -Up for the ceiling).
	 */
	struct SurfaceMesh
	{
		std::vector<StageVec3> Positions;
		std::vector<StageVec2> UVs;
		std::vector<uint32_t> Triangles;
	};

	/** Flat surfaces need one column; curved ones get four per tile column. */
	inline uint32_t RecommendedMeshColumns(const WallSpec& Spec, const ViewportLayout& Viewport)
	{
		if (Viewport.Surface == ESurface::Wall && Spec.CurvatureRadiusM > 0.0)
		{
			return Viewport.TileCountX * 4;
		}
		return 1;
	}

	inline SurfaceMesh BuildViewportMesh(const WallSpec& Spec, const WallLayout& Layout, const ViewportLayout& Viewport, uint32_t Columns, uint32_t Rows = 1)
	{
		SurfaceMesh Mesh;
		Columns = Columns < 1 ? 1 : Columns;
		Rows = Rows < 1 ? 1 : Rows;
		for (uint32_t Row = 0; Row <= Rows; ++Row)
		{
			const double T = static_cast<double>(Row) / static_cast<double>(Rows);
			for (uint32_t Column = 0; Column <= Columns; ++Column)
			{
				const double S = static_cast<double>(Column) / static_cast<double>(Columns);
				const double U = Viewport.U0M + S * (Viewport.U1M - Viewport.U0M);
				const double V = Viewport.V1M - T * (Viewport.V1M - Viewport.V0M);
				Mesh.Positions.push_back(Viewport.Surface == ESurface::Wall ? WallSurfacePoint(Spec, Layout.Wall, U, V) : CeilingSurfacePoint(Spec, Layout, U, V));
				Mesh.UVs.push_back(StageVec2{S, T});
			}
		}
		const uint32_t Stride = Columns + 1;
		for (uint32_t Row = 0; Row < Rows; ++Row)
		{
			for (uint32_t Column = 0; Column < Columns; ++Column)
			{
				const uint32_t TopLeft = Row * Stride + Column;
				const uint32_t TopRight = TopLeft + 1;
				const uint32_t BottomLeft = TopLeft + Stride;
				const uint32_t BottomRight = BottomLeft + 1;
				Mesh.Triangles.insert(Mesh.Triangles.end(), {TopLeft, BottomLeft, BottomRight, TopLeft, BottomRight, TopRight});
			}
		}
		return Mesh;
	}
}
