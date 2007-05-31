# note: all files commented out do not appear in the CONGLOMERATE_SOURCE 
# variable of the original Makefile, but were part of the Makefile and 
# the source tree in other ways


bin_PROGRAMS = \
	add_labels \
	add_surfaces \
	apply_sphere_transform \
	autocrop_volume \
	average_objects \
	average_voxels \
	average_surfaces \
	blur_surface \
	box_filter_volume \
	box_filter_volume_nd \
	chamfer_volume \
	check_polygons \
	chop_tags \
	clamp_volume \
	classify_sulcus \
	clean_surface_labels \
	clip_tags \
	close_surface \
	cluster_volume \
	coalesce_lines \
	colour_object \
	compare_left_right \
	compare_left_right_groups \
	compare_lengths \
	composite_images \
	composite_minc_images \
	composite_volumes \
	compute_bounding_view \
	compute_resels \
	concat_images \
	contour_slice \
	convert_object \
	convex_hull \
	copy_colours \
	count_thresholded_volume \
	create_2d_sheet \
	create_2d_surface \
	create_box \
	create_four_volumes \
	create_grid \
	create_label_map \
	create_landmark_full_volume \
	create_mahalanobis \
	create_rectangle \
	create_surface_interpolation_lsq \
	create_tetra \
	create_warping_points \
	diff_mahalanobis \
	diff_points \
	diff_surfaces \
	dilate_volume \
	dilate_volume_completely \
	dim_image \
	dump_curvatures \
	dump_deformation_distances \
	dump_point_diffs \
	dump_points \
	dump_points_to_tag_file \
	dump_polygons \
	dump_rms \
	dump_transform \
	dump_uv \
	dump_vertex_normal_diffs \
	evaluate \
	extract_tag_slice \
	extract_largest_line \
	fill_sulci \
	find_buried_surface \
	find_image_bounding_box \
	find_peaks \
	find_surface_distances \
	find_tag_outliers \
	find_vertex \
	find_volume_centroid \
	fit_curve \
	fit_curve2 \
	fit_3d \
	flatten_polygons \
	flatten_sheet \
	flatten_sheet3 \
	flatten_to_sphere \
	flatten_to_sphere2 \
	flip_tags \
	flip_volume \
	f_prob \
	gaussian_blur_peaks \
	get_tic \
	group_diff \
	half_polygons \
	histogram_volume \
	intensity_statistics \
	interpolate_tags \
	label_sulci \
	labels_to_rgb \
	lookup_labels \
	make_concentric_surface \
	make_diff_volume \
	make_geodesic_volume \
	make_gradient_volume \
	make_grid_lines \
	make_line_links \
	make_slice \
	make_surface_bitlist \
	make_colour_bar \
	make_sphere_transform \
	manifold_polygons \
	map_colours_to_sphere \
	map_surface_to_sheet \
	map_sheets \
	marching_cubes \
	mask_values \
	mask_volume \
	match_tags \
	measure_surface_area \
	merge_polygons \
	mincdefrag \
	mincmask \
	mincskel \
	minctotag \
	minc_to_rgb \
	normalize_pet \
	perturb_surface \
	place_images \
	plane_polygon_intersect \
	polygon_map \
	polygons_to_lines \
	preprocess_segmentation \
	print_2d_coords \
	print_all_labels \
	print_all_label_bounding_boxes \
	print_axis_angles \
	print_n_polygons \
	print_object_centroid \
	print_object_limits \
	print_volume_value \
	print_world_value \
	print_world_values \
	random_warp \
	reconstitute_points \
	refine_mesh \
	reparameterize_line \
	rgb_to_minc \
	scale_minc_image \
	scan_lines_to_polygons \
	scan_object_to_volume \
	segment_probabilities \
	separate_polygons \
	set_line_width \
	set_object_colour \
	set_object_opacity \
	set_object_surfprop \
	smooth_lines \
	smooth_normals \
	spherical_resample \
	spline_lines \
	split_polygons \
	stats_tag_file \
	subsample_volume \
	subdivide_polygons \
	subdivide_values \
	surface_mask \
	surface_mask2 \
	tag_volume \
	tagtominc \
	threshold_volume \
	triangulate_polygons \
	trimesh_resample \
	trimesh_set_points \
	trimesh_to_polygons \
	tags_to_spheres \
	transform_volume \
	transform_tags \
	transform_objects \
	two_surface_resample \
	volume_object_evaluate

bin_SCRIPTS = \
	nlfit_smr \
	stx_segment \
	mritotal_suppress \
	smooth_mask \
	compute_icbm_vols \
	multispectral_stx_registration \
	suppress_fat \
	regional_thickness \
	remap_to_lobes

AUTOMAKE_OPTIONS = check-news
ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = $(m4_files) \
	epm-header.in \
	nlfit_smr.in \
	stx_segment.in \
	mritotal_suppress.in \
	smooth_mask.in \
	compute_icbm_vols.in \
	multispectral_stx_registration.in \
	suppress_fat.in \
	regional_thickness.in \
	remap_to_lobes.in

CLEANFILES = $(bin_SCRIPTS)

# These headers are used in one or more of the source files.
# Eventually the relevant _SOURCES lines should contain the header
# files.  Until then, this explicit list allows us to build a distribution.
noinst_HEADERS = \
	conjugate_grad.h \
	conjugate_grad_prototypes.h \
	conjugate_min.h \
	conjugate_min_prototypes.h \
	deform.h \
	deform_prototypes.h \
	interval.h \
	line_min_prototypes.h \
	mi_label_prototypes.h \
	minc_labels.h \
	sp_geom_prototypes.h \
	special_geometry.h \
	tri_mesh.h

m4_files = m4/mni_REQUIRE_LIB.m4 \
           m4/mni_REQUIRE_MNILIBS.m4 \
           m4/mni_REQUIRE_OPENINVENTOR.m4 \
           m4/mni_cxx_have_koenig_lookup.m4 \
           m4/smr_CGAL_MAKEFILE.m4 \
           m4/smr_OPTIONAL_LIB.m4 \
           m4/smr_REQUIRED_LIB.m4 \
           m4/smr_WITH_BUILD_PATH.m4

add_labels_SOURCES =  add_labels.c minc_labels.c
add_surfaces_SOURCES =  add_surfaces.c
apply_sphere_transform_SOURCES =  apply_sphere_transform.c
autocrop_volume_SOURCES =  autocrop_volume.c
average_objects_SOURCES =  average_objects.c
average_surfaces_SOURCES =  average_surfaces.c
average_voxels_SOURCES =  average_voxels.c
blur_surface_SOURCES =  blur_surface.c
box_filter_volume_nd_SOURCES =  box_filter_volume_nd.c
box_filter_volume_SOURCES =  box_filter_volume.c
chamfer_volume_SOURCES =  chamfer_volume.c
check_polygons_SOURCES =  check_polygons.c
chop_tags_SOURCES =  chop_tags.c
clamp_volume_SOURCES =  clamp_volume.c
classify_sulcus_SOURCES =  classify_sulcus.c
clean_surface_labels_SOURCES = clean_surface_labels.c
clip_tags_SOURCES =  clip_tags.c
close_surface_SOURCES =  close_surface.c
cluster_volume_SOURCES =  cluster_volume.c
coalesce_lines_SOURCES =  coalesce_lines.c
colour_object_SOURCES =  colour_object.c
compare_left_right_groups_SOURCES =  compare_left_right_groups.c
compare_left_right_SOURCES =  compare_left_right.c
compare_lengths_SOURCES =  compare_lengths.c
composite_images_SOURCES =  composite_images.c
composite_minc_images_SOURCES =  composite_minc_images.c
composite_volumes_SOURCES =  composite_volumes.c
compute_bounding_view_SOURCES =  compute_bounding_view.c
compute_resels_SOURCES =  compute_resels.c
concat_images_SOURCES =  concat_images.c
contour_slice_SOURCES =  contour_slice.c
convert_object_SOURCES =  convert_object.c
convex_hull_SOURCES = convex_hull.c
copy_colours_SOURCES =  copy_colours.c
count_thresholded_volume_SOURCES =  count_thresholded_volume.c
create_2d_sheet_SOURCES =  create_2d_sheet.c
create_2d_surface_SOURCES =  create_2d_surface.c
create_box_SOURCES =  create_box.c
create_four_volumes_SOURCES =  create_four_volumes.c
create_grid_SOURCES =  create_grid.c
create_label_map_SOURCES =  create_label_map.c
create_landmark_full_volume_SOURCES =  create_landmark_full_volume.c get_filenames.c
create_mahalanobis_SOURCES =  create_mahalanobis.c
create_rectangle_SOURCES =  create_rectangle.c
create_surface_interpolation_lsq_SOURCES =  create_surface_interpolation_lsq.c
create_tetra_SOURCES =  create_tetra.c
create_warping_points_SOURCES =  create_warping_points.c
diff_mahalanobis_SOURCES =  diff_mahalanobis.c
diff_points_SOURCES =  diff_points.c
diff_surfaces_SOURCES =  diff_surfaces.c
dilate_volume_completely_SOURCES =  dilate_volume_completely.c
dilate_volume_SOURCES =  dilate_volume.c
dim_image_SOURCES =  dim_image.c
dump_curvatures_SOURCES =  dump_curvatures.c
dump_deformation_distances_SOURCES =  dump_deformation_distances.c
dump_point_diffs_SOURCES =  dump_point_diffs.c
dump_points_SOURCES =  dump_points.c
dump_points_to_tag_file_SOURCES =  dump_points_to_tag_file.c
dump_polygons_SOURCES =  dump_polygons.c
dump_rms_SOURCES =  dump_rms.c
dump_transform_SOURCES =  dump_transform.c
dump_uv_SOURCES =  dump_uv.c
dump_vertex_normal_diffs_SOURCES =  dump_vertex_normal_diffs.c
evaluate_SOURCES =  evaluate.c
extract_largest_line_SOURCES =  extract_largest_line.c
extract_tag_slice_SOURCES =  extract_tag_slice.c
fill_sulci_SOURCES =  fill_sulci.c
find_buried_surface_SOURCES =  find_buried_surface.c
find_image_bounding_box_SOURCES =  find_image_bounding_box.c
find_peaks_SOURCES = find_peaks.c
find_surface_distances_SOURCES =  find_surface_distances.c search_utils.c find_in_direction.c model_objects.c intersect_voxel.c deform_line.c models.c
find_tag_outliers_SOURCES =  find_tag_outliers.c
find_vertex_SOURCES =  find_vertex.c
find_volume_centroid_SOURCES =  find_volume_centroid.c
fit_3d_SOURCES =  fit_3d.c find_in_direction.c model_objects.c intersect_voxel.c deform_line.c models.c search_utils.c
fit_curve2_SOURCES =  fit_curve2.c  conjugate_min.c conjugate_grad.c line_minimization.c
fit_curve_SOURCES =  fit_curve.c
flatten_polygons_SOURCES =  flatten_polygons.c
flatten_sheet3_SOURCES =  flatten_sheet3.c
flatten_sheet_SOURCES =  flatten_sheet.c
flatten_to_sphere2_SOURCES =  flatten_to_sphere2.c
flatten_to_sphere_SOURCES =  flatten_to_sphere.c
flip_tags_SOURCES =  flip_tags.c
flip_volume_SOURCES =  flip_volume.c
f_prob_SOURCES =  f_prob.c
gaussian_blur_peaks_SOURCES =  gaussian_blur_peaks.c
get_tic_SOURCES =  get_tic.c
group_diff_SOURCES =  group_diff.c
half_polygons_SOURCES =  half_polygons.c
histogram_volume_SOURCES =  histogram_volume.c
intensity_statistics_SOURCES =  intensity_statistics.c
interpolate_tags_SOURCES =  interpolate_tags.c
labels_to_rgb_SOURCES =  labels_to_rgb.c
label_sulci_SOURCES =  label_sulci.c
lookup_labels_SOURCES =  lookup_labels.c minc_labels.c
make_colour_bar_SOURCES =  make_colour_bar.c
make_concentric_surface_SOURCES =  make_concentric_surface.c
make_diff_volume_SOURCES =  make_diff_volume.c
make_geodesic_volume_SOURCES =  make_geodesic_volume.c
make_gradient_volume_SOURCES =  make_gradient_volume.c
make_grid_lines_SOURCES =  make_grid_lines.c
make_line_links_SOURCES =  make_line_links.c
make_slice_SOURCES =  make_slice.c
make_sphere_transform_SOURCES =  make_sphere_transform.c
make_surface_bitlist_SOURCES =  make_surface_bitlist.c
manifold_polygons_SOURCES =  manifold_polygons.c
map_colours_to_sphere_SOURCES =  map_colours_to_sphere.c
map_sheets_SOURCES =  map_sheets.c
map_surface_to_sheet_SOURCES =  map_surface_to_sheet.c
mask_values_SOURCES =  mask_values.c
mask_volume_SOURCES =  mask_volume.c
match_tags_SOURCES = match_tags.c
measure_surface_area_SOURCES =  measure_surface_area.c
merge_polygons_SOURCES =  merge_polygons.c
minc_to_rgb_SOURCES =  minc_to_rgb.c
mincdefrag_SOURCES = mincdefrag.cc
mincmask_SOURCES = mincmask.c
mincskel_SOURCES = mincskel.cc
minctotag_SOURCES =  minctotag.c
normalize_pet_SOURCES = normalize_pet.c
perturb_surface_SOURCES =  perturb_surface.c
place_images_SOURCES =  place_images.c
plane_polygon_intersect_SOURCES =  plane_polygon_intersect.c
polygon_map_SOURCES =  polygon_map.c
polygons_to_lines_SOURCES =  polygons_to_lines.c
preprocess_segmentation_SOURCES =  preprocess_segmentation.c
print_2d_coords_SOURCES =  print_2d_coords.c
print_all_label_bounding_boxes_SOURCES =  print_all_label_bounding_boxes.c
print_all_labels_SOURCES =  print_all_labels.c
print_axis_angles_SOURCES =  print_axis_angles.c
print_n_polygons_SOURCES =  print_n_polygons.c
print_object_centroid_SOURCES =  print_object_centroid.c
print_object_limits_SOURCES =  print_object_limits.c
print_volume_value_SOURCES =  print_volume_value.c
print_world_value_SOURCES =  print_world_value.c
print_world_values_SOURCES =  print_world_values.c
random_warp_SOURCES =  random_warp.c
reconstitute_points_SOURCES =  reconstitute_points.c
refine_mesh_SOURCES =  refine_mesh.c
reparameterize_line_SOURCES =  reparameterize_line.c
rgb_to_minc_SOURCES =  rgb_to_minc.c
scale_minc_image_SOURCES =  scale_minc_image.c
scan_lines_to_polygons_SOURCES =  scan_lines_to_polygons.c
scan_object_to_volume_SOURCES =  scan_object_to_volume.c
segment_probabilities_SOURCES =  segment_probabilities.c
separate_polygons_SOURCES =  separate_polygons.c
set_line_width_SOURCES =  set_line_width.c
set_object_colour_SOURCES =  set_object_colour.c
set_object_opacity_SOURCES =  set_object_opacity.c
smooth_lines_SOURCES =  smooth_lines.c
smooth_normals_SOURCES =  smooth_normals.c
spherical_resample_SOURCES =  spherical_resample.c
spline_lines_SOURCES =  spline_lines.c
split_polygons_SOURCES =  split_polygons.c
stats_tag_file_SOURCES =  stats_tag_file.c
subdivide_polygons_SOURCES =  subdivide_polygons.c
subdivide_values_SOURCES =  subdivide_values.c
subsample_volume_SOURCES =  subsample_volume.c
surface_mask2_SOURCES =  surface_mask2.c
surface_mask_SOURCES =  surface_mask.c
tags_to_spheres_SOURCES =  tags_to_spheres.c
tagtominc_SOURCES =  tagtominc.c
tag_volume_SOURCES =  tag_volume.c
threshold_volume_SOURCES =  threshold_volume.c
transform_objects_SOURCES =  transform_objects.c
transform_tags_SOURCES =  transform_tags.c
transform_volume_SOURCES =  transform_volume.c
triangulate_polygons_SOURCES =  triangulate_polygons.c
trimesh_resample_SOURCES =  trimesh_resample.c tri_mesh.c
trimesh_set_points_SOURCES =  trimesh_set_points.c tri_mesh.c
trimesh_to_polygons_SOURCES =  trimesh_to_polygons.c tri_mesh.c
two_surface_resample_SOURCES =  two_surface_resample.c
volume_object_evaluate_SOURCES = volume_object_evaluate.c

