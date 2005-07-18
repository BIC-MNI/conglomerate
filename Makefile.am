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
	mincmask \
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
	suppress_fat

do_subst = sed -e 's,[@]datadir[@],$(datadir),g' \
	-e 's,[@]PERL[@],$(PERL),g' \
	-e 's,[@]PACKAGE[@],$(PACKAGE),g' \
	-e 's,[@]VERSION[@],$(VERSION),g'

nlfit_smr: nlfit_smr.pl Makefile
	$(do_subst) < $(srcdir)/$< > $@
	chmod +x $@

stx_segment: stx_segment.pl Makefile
	$(do_subst) < $(srcdir)/$< > $@
	chmod +x $@

mritotal_suppress: mritotal_suppress.pl Makefile
	$(do_subst) < $(srcdir)/$< > $@
	chmod +x $@

compute_icbm_vols: compute_icbm_vols.pl Makefile
	$(do_subst) < $(srcdir)/$< > $@
	chmod +x $@

smooth_mask: smooth_mask.pl Makefile
	$(do_subst) < $(srcdir)/$< > $@
	chmod +x $@

multispectral_stx_registration: multispectral_stx_registration.pl Makefile
	$(do_subst) < $(srcdir)/$< > $@
	chmod +x $@

suppress_fat: suppress_fat.pl Makefile
	$(do_subst) < $(srcdir)/$< > $@
	chmod +x $@

AUTOMAKE_OPTIONS = check-news
ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = $(m4_files) \
	epm-header.in \
	nlfit_smr.pl \
	stx_segment.pl \
	mritotal_suppress.pl \
	smooth_mask.pl \
	compute_icbm_vols.pl \
	multispectral_stx_registration.pl \
	suppress_fat.pl

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


m4_files = \
	m4/mni_REQUIRE_LIB.m4 \
	m4/mni_REQUIRE_MNILIBS.m4 \
	m4/smr_WITH_BUILD_PATH.m4

add_labels_SOURCES =  add_labels.c minc_labels.c
add_surfaces_SOURCES =  add_surfaces.c
#analyze_grid_SOURCES =  analyze_grid.c
apply_sphere_transform_SOURCES =  apply_sphere_transform.c
#arbitrary_subdivide_SOURCES =  arbitrary_subdivide.c
autocrop_volume_SOURCES =  autocrop_volume.c
average_objects_SOURCES =  average_objects.c
average_surfaces_SOURCES =  average_surfaces.c
average_voxels_SOURCES =  average_voxels.c
#basic_stats_SOURCES =  basic_stats.c
blur_surface_SOURCES =  blur_surface.c
box_filter_volume_nd_SOURCES =  box_filter_volume_nd.c
box_filter_volume_SOURCES =  box_filter_volume.c
#cards_SOURCES =  cards.c
chamfer_volume_SOURCES =  chamfer_volume.c
#change_for_eva_SOURCES =  change_for_eva.c
#change_image_colour_SOURCES =  change_image_colour.c
check_polygons_SOURCES =  check_polygons.c
#check_tags_SOURCES =  check_tags.c
chop_tags_SOURCES =  chop_tags.c
clamp_volume_SOURCES =  clamp_volume.c
classify_sulcus_SOURCES =  classify_sulcus.c
#clip_polygons_SOURCES =  clip_polygons.c
clip_tags_SOURCES =  clip_tags.c
close_surface_SOURCES =  close_surface.c
cluster_volume_SOURCES =  cluster_volume.c
coalesce_lines_SOURCES =  coalesce_lines.c
#colour_curvature_SOURCES =  colour_curvature.c
colour_object_SOURCES =  colour_object.c
compare_left_right_groups_SOURCES =  compare_left_right_groups.c
compare_left_right_SOURCES =  compare_left_right.c
compare_lengths_SOURCES =  compare_lengths.c
#compare_volumes_SOURCES =  compare_volumes.c
composite_images_SOURCES =  composite_images.c
composite_minc_images_SOURCES =  composite_minc_images.c
composite_volumes_SOURCES =  composite_volumes.c
compute_bounding_view_SOURCES =  compute_bounding_view.c
compute_resels_SOURCES =  compute_resels.c
concat_images_SOURCES =  concat_images.c
contour_slice_SOURCES =  contour_slice.c
#convert_rgb_SOURCES =  convert_rgb.c
#convex_hull_SOURCES =  convex_hull.c
copy_colours_SOURCES =  copy_colours.c
#count_region_SOURCES =  count_region.c
#count_tags_SOURCES =  count_tags.c
count_thresholded_volume_SOURCES =  count_thresholded_volume.c
create_2d_sheet_SOURCES =  create_2d_sheet.c
create_2d_surface_SOURCES =  create_2d_surface.c
create_box_SOURCES =  create_box.c
#create_ellipse_SOURCES =  create_ellipse.c
#create_ellipsoids_SOURCES =  create_ellipsoids.c
create_four_volumes_SOURCES =  create_four_volumes.c
#create_function_volume_SOURCES =  create_function_volume.c
create_grid_SOURCES =  create_grid.c
create_label_map_SOURCES =  create_label_map.c
create_landmark_full_volume_SOURCES =  create_landmark_full_volume.c get_filenames.c
#create_landmark_volume_SOURCES =  create_landmark_volume.c
create_mahalanobis_SOURCES =  create_mahalanobis.c
#create_marching_coords_SOURCES =  create_marching_coords.c
create_rectangle_SOURCES =  create_rectangle.c
#create_sphere_SOURCES =  create_sphere.c
#create_sphere_volume_SOURCES =  create_sphere_volume.c
create_surface_interpolation_lsq_SOURCES =  create_surface_interpolation_lsq.c
create_tetra_SOURCES =  create_tetra.c
#create_tetra_test_SOURCES =  create_tetra.c sp.c poly_neighs.c
#create_volume_SOURCES =  create_volume.c
create_warping_points_SOURCES =  create_warping_points.c
#cross_correlation_3d_SOURCES =  cross_correlation_3d.c
#deform_surface_SOURCES =  deform_surface.c
#delauney_SOURCES =  delauney.c
diff_mahalanobis_SOURCES =  diff_mahalanobis.c
diff_points_SOURCES =  diff_points.c
diff_surfaces_SOURCES =  diff_surfaces.c
#dilate_image_SOURCES =  dilate_image.c
dilate_volume_completely_SOURCES =  dilate_volume_completely.c
dilate_volume_SOURCES =  dilate_volume.c
dim_image_SOURCES =  dim_image.c
#distance_surfaces_haus_SOURCES =  distance_surfaces_haus.c
#distance_surfaces_SOURCES =  distance_surfaces.c
dump_curvatures_SOURCES =  dump_curvatures.c
dump_deformation_distances_SOURCES =  dump_deformation_distances.c
#dump_mesh_SOURCES =  dump_mesh.c
dump_point_diffs_SOURCES =  dump_point_diffs.c
dump_points_SOURCES =  dump_points.c
dump_points_to_tag_file_SOURCES =  dump_points_to_tag_file.c
dump_polygons_SOURCES =  dump_polygons.c
dump_rms_SOURCES =  dump_rms.c
#dump_slice_SOURCES =  dump_slice.c
#dump_tal_SOURCES =  dump_tal.c
dump_transform_SOURCES =  dump_transform.c
dump_uv_SOURCES =  dump_uv.c
#dump_vertex_SOURCES =  dump_vertex.c
#embed_in_sphere_SOURCES =  embed_in_sphere.c
#equalize_distance2_SOURCES =  equalize_distance2.c
#equalize_distance3_SOURCES =  equalize_distance3.c
#equalize_distance_SOURCES =  equalize_distance.c
#evaluate_cross_correlation_SOURCES =  evaluate_cross_correlation.c
evaluate_SOURCES =  evaluate.c
#evaluate_spline_SOURCES =  evaluate_spline.c evaluate_basis.c cubic_approximation.c
#examine_polygons_SOURCES =  examine_polygons.c
#expand_polygons_slightly_SOURCES =  expand_polygons_slightly.c
#extend_slice_SOURCES =  extend_slice.c
extract_largest_line_SOURCES =  extract_largest_line.c
#extract_points_SOURCES =  extract_points.c
#extract_sulci_SOURCES =  extract_sulci.c
extract_tag_slice_SOURCES =  extract_tag_slice.c
#extrude_lines_SOURCES =  extrude_lines.c
#fill_image_SOURCES =  fill_image.c
fill_sulci_SOURCES =  fill_sulci.c
#film_loop_SOURCES =  film_loop.c
#filter_volume_SOURCES =  filter_volume.c
#find_buried_surface2_SOURCES =  find_buried_surface2.c
find_buried_surface_SOURCES =  find_buried_surface.c
#find_hemi_SOURCES =  find_hemi.c
find_image_bounding_box_SOURCES =  find_image_bounding_box.c
#find_interruptions_SOURCES =  find_interruptions.c
find_peaks_SOURCES = find_peaks.c
#find_polygon_containing_point_SOURCES =  find_polygon_containing_point.c
find_surface_distances_SOURCES =  find_surface_distances.c search_utils.c find_in_direction.c model_objects.c intersect_voxel.c deform_line.c models.c
find_tag_outliers_SOURCES =  find_tag_outliers.c
find_vertex_SOURCES =  find_vertex.c
find_volume_centroid_SOURCES =  find_volume_centroid.c
#fit_2d_SOURCES =  fit_2d.c
fit_3d_SOURCES =  fit_3d.c find_in_direction.c model_objects.c intersect_voxel.c deform_line.c models.c search_utils.c
fit_curve2_SOURCES =  fit_curve2.c  conjugate_min.c conjugate_grad.c line_minimization.c
fit_curve_SOURCES =  fit_curve.c
#fit_line_SOURCES =  fit_line.c
flatten_polygons_SOURCES =  flatten_polygons.c
#flatten_sheet2_SOURCES =  flatten_sheet2.c
flatten_sheet3_SOURCES =  flatten_sheet3.c
flatten_sheet_SOURCES =  flatten_sheet.c
flatten_to_sphere2_SOURCES =  flatten_to_sphere2.c
#flatten_to_sphere3_SOURCES =  flatten_to_sphere3.c
#flatten_to_sphere4_SOURCES =  flatten_to_sphere4.c
#flatten_to_sphere5_SOURCES =  flatten_to_sphere5.c
#flatten_to_sphere6_SOURCES =  flatten_to_sphere6.c
#flatten_to_sphere7_SOURCES =  flatten_to_sphere7.c
#flatten_to_sphere8_SOURCES =  flatten_to_sphere8.c
#flatten_to_sphere9_SOURCES =  flatten_to_sphere9.c
flatten_to_sphere_SOURCES =  flatten_to_sphere.c
#flip_activity_SOURCES =  flip_activity.c
flip_tags_SOURCES =  flip_tags.c
#flip_talairach_brain_SOURCES =  flip_talairach_brain.c
flip_volume_SOURCES =  flip_volume.c
f_prob_SOURCES =  f_prob.c
#frequencies_SOURCES =  frequencies.c
gaussian_blur_peaks_SOURCES =  gaussian_blur_peaks.c
get_tic_SOURCES =  get_tic.c
#gradient_histogram_SOURCES =  gradient_histogram.c
#grid_transform_SOURCES =  grid_transform.c
group_diff_SOURCES =  group_diff.c
half_polygons_SOURCES =  half_polygons.c
#histogram_volume2_SOURCES =  histogram_volume2.c
histogram_volume_SOURCES =  histogram_volume.c
#image_io_SOURCES =  image_io.c
intensity_statistics_SOURCES =  intensity_statistics.c
interpolate_tags_SOURCES =  interpolate_tags.c
#label_inside_surface_SOURCES =  label_inside_surface.c
labels_to_rgb_SOURCES =  labels_to_rgb.c
label_sulci_SOURCES =  label_sulci.c
#left_minus_right_SOURCES =  left_minus_right.c
#lint_deform_surface_SOURCES =  deform_surface.ln
#lmk_to_tag_SOURCES =  lmk_to_tag.c
lookup_labels_SOURCES =  lookup_labels.c minc_labels.c
#make_backdrop_SOURCES =  make_backdrop.c
make_colour_bar_SOURCES =  make_colour_bar.c
#make_columns_SOURCES =  make_columns.c
make_concentric_surface_SOURCES =  make_concentric_surface.c
#make_core_volume_SOURCES =  make_core_volume.c
make_diff_volume_SOURCES =  make_diff_volume.c
#make_edge_points_SOURCES =  make_edge_points.c
#make_edges_SOURCES =  make_edges.c
make_geodesic_volume_SOURCES =  make_geodesic_volume.c
#make_gradient_arrows_SOURCES =  make_gradient_arrows.c
make_gradient_volume_SOURCES =  make_gradient_volume.c
make_grid_lines_SOURCES =  make_grid_lines.c
#make_histogram_volume_SOURCES =  make_histogram_volume.c
#make_labels_SOURCES =  make_labels.c
make_line_links_SOURCES =  make_line_links.c
#make_max_grad_SOURCES =  make_max_grad.c
#make_normals_SOURCES =  read_write.c make_normals.c
make_slice_SOURCES =  make_slice.c
#make_slice_volume_SOURCES =  make_slice_volume.c
make_sphere_transform_SOURCES =  make_sphere_transform.c
make_surface_bitlist_SOURCES =  make_surface_bitlist.c
#manifold_labels_SOURCES =  manifold_labels.c
#manifold_polygons2_SOURCES =  manifold_polygons2.c
manifold_polygons_SOURCES =  manifold_polygons.c
map_colours_to_sphere_SOURCES =  map_colours_to_sphere.c
#map_objects_to_sheet_SOURCES =  map_objects_to_sheet.c
map_sheets_SOURCES =  map_sheets.c
map_surface_to_sheet_SOURCES =  map_surface_to_sheet.c
mask_values_SOURCES =  mask_values.c
#mask_volume_4d_SOURCES =  mask_volume_4d.c
mask_volume_SOURCES =  mask_volume.c
match_tags_SOURCES = match_tags.c
#match_histograms_SOURCES =  match_histograms.c
#matrix1_SOURCES =  matrix1.c
measure_surface_area_SOURCES =  measure_surface_area.c
merge_polygons_SOURCES =  merge_polygons.c
minc_to_rgb_SOURCES =  minc_to_rgb.c
mincmask_SOURCES = mincmask.c
minctotag_SOURCES =  minctotag.c
#normalize_intensity_SOURCES =  normalize_intensity.c
normalize_pet_SOURCES = normalize_pet.c
#obj_to_rgb_SOURCES =  obj_to_rgb.c
#offset_volume2_SOURCES =  offset_volume2.c
#offset_volume_SOURCES =  offset_volume.c
#open_manifold_SOURCES =  open_manifold.c
#perspective_solution_SOURCES =  perspective_solution.c
#perturb_labels_SOURCES =  perturb_labels.c
perturb_surface_SOURCES =  perturb_surface.c
place_images_SOURCES =  place_images.c
plane_polygon_intersect_SOURCES =  plane_polygon_intersect.c
polygon_map_SOURCES =  polygon_map.c
polygons_to_lines_SOURCES =  polygons_to_lines.c
#polygon_volume_SOURCES =  polygon_volume.c
preprocess_segmentation_SOURCES =  preprocess_segmentation.c
#prime_SOURCES =  prime.c
print_2d_coords_SOURCES =  print_2d_coords.c
print_all_label_bounding_boxes_SOURCES =  print_all_label_bounding_boxes.c
print_all_labels_SOURCES =  print_all_labels.c
print_axis_angles_SOURCES =  print_axis_angles.c
print_n_polygons_SOURCES =  print_n_polygons.c
print_object_centroid_SOURCES =  print_object_centroid.c
print_object_limits_SOURCES =  print_object_limits.c
print_volume_value_SOURCES =  print_volume_value.c
print_world_value_SOURCES =  print_world_value.c
random_warp_SOURCES =  random_warp.c
reconstitute_points_SOURCES =  reconstitute_points.c
refine_mesh_SOURCES =  refine_mesh.c
#remove_singles_SOURCES =  remove_singles.c
#remove_tags_SOURCES =  remove_tags.c
#reorder_volume_SOURCES =  reorder_volume.c
reparameterize_line_SOURCES =  reparameterize_line.c
#resample_triangulation_SOURCES =  resample_triangulation.c
#rf_volume_SOURCES =  rf_volume.c
rgb_to_minc_SOURCES =  rgb_to_minc.c
#rotate_objects_SOURCES =  rotate_objects.c
scale_minc_image_SOURCES =  scale_minc_image.c
#scale_volume_SOURCES =  scale_volume.c
scan_lines_to_polygons_SOURCES =  scan_lines_to_polygons.c
scan_object_to_volume_SOURCES =  scan_object_to_volume.c
#segment_callosum_SOURCES =  segment_callosum.c
#segment_polygons_SOURCES =  segment_polygons.c
segment_probabilities_SOURCES =  segment_probabilities.c
separate_polygons_SOURCES =  separate_polygons.c
set_line_width_SOURCES =  set_line_width.c
set_object_colour_SOURCES =  set_object_colour.c
set_object_opacity_SOURCES =  set_object_opacity.c
#similarity_volume_SOURCES =  similarity_volume.c
#smooth_dilate_volume_SOURCES =  smooth_dilate_volume.c
smooth_lines_SOURCES =  smooth_lines.c
smooth_normals_SOURCES =  smooth_normals.c
#solve_camera_SOURCES =  solve_camera.c
#solve_view_SOURCES =  solve_view.c
#sort_volume_SOURCES =  sort_volume.c
spherical_resample_SOURCES =  spherical_resample.c
spline_lines_SOURCES =  spline_lines.c
#split_file_SOURCES =  split_file.c
#split_images_SOURCES =  split_images.c
split_polygons_SOURCES =  split_polygons.c
stats_tag_file_SOURCES =  stats_tag_file.c
subdivide_polygons_SOURCES =  subdivide_polygons.c
#subdivide_polygons_test_SOURCES =  subdivide_polygons.c poly_neighs.c sp.c
subdivide_values_SOURCES =  subdivide_values.c
#subsample_polygons_SOURCES =  subsample_polygons.c
subsample_volume_SOURCES =  subsample_volume.c
surface_mask2_SOURCES =  surface_mask2.c
surface_mask_SOURCES =  surface_mask.c
#tag_mask_SOURCES =  tag_mask.c
#tag_statistics_SOURCES =  tag_statistics.c
tags_to_spheres_SOURCES =  tags_to_spheres.c
tagtominc_SOURCES =  tagtominc.c
#tagtoxfm_SOURCES =  tagtoxfm.c
tag_volume_SOURCES =  tag_volume.c
#test_arrays_SOURCES =  test_arrays.c
#test_c_SOURCES =  test_c.c
#test_deriv_SOURCES =  test_deriv.c
#test_dynamic_SOURCES =  test_dynamic.c
#test_hyperslab_SOURCES =  test_hyperslab.c
#test_labels_SOURCES =  test_labels.c
#test_label_volume_SOURCES =  test_label_volume.c labels.c
#test_map_SOURCES =  test_map.c
#test_point_polygon_distance_SOURCES =  test_point_polygon_distance.c
#test_ray_intersect_SOURCES =  test_ray_intersect.c
#test_smooth_SOURCES =  conjugate_grad_prototypes.h conjugate_min_prototypes.h line_min_prototypes.h test_smooth.c conjugate_min.c conjugate_grad.c line_minimization.c
#test_SOURCES =  test.c
#test_swap_SOURCES =  test_swap.c
#test_tags_SOURCES =  test_tags.c
#test_view_SOURCES =  test_view.c
threshold_volume_SOURCES =  threshold_volume.c
#transform_labels_SOURCES =  transform_labels.c
transform_objects_SOURCES =  transform_objects.c
transform_tags_SOURCES =  transform_tags.c
#transform_to_world_SOURCES =  transform_to_world.c
transform_volume_SOURCES =  transform_volume.c
triangulate_polygons_SOURCES =  triangulate_polygons.c
trimesh_resample_SOURCES =  trimesh_resample.c tri_mesh.c
trimesh_set_points_SOURCES =  trimesh_set_points.c tri_mesh.c
trimesh_to_polygons_SOURCES =  trimesh_to_polygons.c tri_mesh.c
#tristrip_polygons_SOURCES =  tristrip_polygons.c
#t_stat_to_probability_SOURCES =  t_stat_to_probability.c
#tubify_SOURCES =  tubify.c
two_surface_resample_SOURCES =  two_surface_resample.c
#vol2roi_SOURCES =  vol2roi.c
#volume_colour_SOURCES =  volume_colour.c
volume_object_evaluate_SOURCES = volume_object_evaluate.c
#voxelate_landmarks_SOURCES =  voxelate_landmarks.c
#zoom_gradient_SOURCES =  zoom_gradient.c



#conjugate_grad_prototypes.h_SOURCES =  conjugate_grad.c
#	@echo "#ifndef  DEF_CONJUGATE_GRAD_PROTOTYPES"                 >  $@
#	@echo "#define  DEF_CONJUGATE_GRAD_PROTOTYPES"                 >> $@
#	@extract_functions -public conjugate_grad.c                    >> $@
#	@echo "#endif"                                                 >> $@

#conjugate_min_prototypes.h_SOURCES =  conjugate_min.c
#	@echo "#ifndef  DEF_CONJUGATE_MIN_PROTOTYPES"                  >  $@
#	@echo "#define  DEF_CONJUGATE_MIN_PROTOTYPES"                  >> $@
#	@extract_functions -public conjugate_min.c                     >> $@
#	@echo "#endif"                                                 >> $@

#line_min_prototypes.h_SOURCES =  line_minimization.c
#	@echo "#ifndef  DEF_LINE_MINIMIZATION_PROTOTYPES"              >  $@
#	@echo "#define  DEF_LINE_MINIMIZATION_PROTOTYPES"              >> $@
#	@extract_functions -public line_minimization.c                 >> $@
#	@echo "#endif"                                                 >> $@

#test_conjugate_SOURCES =  conjugate_grad_prototypes.h conjugate_min_prototypes.h \
#                line_min_prototypes.h \
#                test_conjugate.c conjugate_min.c conjugate_grad.c \
#                line_minimization.c
#	$(CC) $(LFLAGS) test_conjugate.c conjugate_min.c conjugate_grad.c \
#                        line_minimization.c $(LIBS) -o $@

#test_new_min_SOURCES =  test_new_min.c
#	$(CC) $(LFLAGS) test_new_min.c $(LIBS) -o $@

#fit_volumes_SOURCES =  conjugate_grad_prototypes.h conjugate_min_prototypes.h \
#                line_min_prototypes.h \
#                fit_volumes.c conjugate_min.c conjugate_grad.c \
#                line_minimization.c
#	$(CC) $(LFLAGS) fit_volumes.c conjugate_min.c conjugate_grad.c \
#                        line_minimization.c $(LIBS) -o $@
