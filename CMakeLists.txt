# CMakeFiles.txt for the classify
#
# Vladimir S. FONOV - vladimir.fonov@gmail.com

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(conglomerate)

#SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake-modules")
# ENABLE_TESTING()
# INCLUDE(CTest)

#ADD_SUBDIRECTORY( doc )

# Packaging defines
SET(CPACK_GENERATOR TGZ)
SET(CPACK_PACKAGE_VERSION_MAJOR 1)
SET(CPACK_PACKAGE_VERSION_MINOR 6)
SET(CPACK_PACKAGE_VERSION_PATCH 60)
INCLUDE(CPack)


SET(PACKAGE "conglomerate")
SET(PACKAGE_BUGREPORT "a.janke@gmail.com")

SET(PACKAGE_NAME "conglomerate")
SET(PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
SET(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")

FIND_PACKAGE( MINC2 REQUIRED )
FIND_PACKAGE( BICPL REQUIRED )
FIND_PACKAGE( Perl REQUIRED )
INCLUDE( ${MINC2_USE_FILE} )
INCLUDE( ${BICPL_USE_FILE} )
LINK_LIBRARIES(${BICPL_LIBRARIES} ${VOLUME_IO_LIBRARIES} ${MINC2_LIBRARIES})

ADD_DEFINITIONS(-DHAVE_CONFIG_H)

# add for building relocatable library
# IF(UNIX)
#   SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
#   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
# ENDIF(UNIX)

# aliases
SET(VERSION "${PACKAGE_VERSION}")


# check for prereqs
# INCLUDE(CheckFunctionExists)
INCLUDE(CheckIncludeFiles)

# CHECK_FUNCTION_EXISTS(finite   HAVE_FINITE)
# CHECK_FUNCTION_EXISTS(isfinite HAVE_ISFINITE)

CHECK_INCLUDE_FILES(sys/dir.h   HAVE_SYS_DIR_H)
CHECK_INCLUDE_FILES(sys/stat.h  HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILES(sys/types.h HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILES(unistd.h    HAVE_UNISTD_H)
CHECK_INCLUDE_FILES(memory.h    HAVE_MEMORY_H)
CHECK_INCLUDE_FILES(malloc.h    HAVE_MALLOC_H)
CHECK_INCLUDE_FILES(stdlib.h    HAVE_STDLIB_H)
CHECK_INCLUDE_FILES(inttypes.h  HAVE_INTTYPES_H)
CHECK_INCLUDE_FILES(string.h    HAVE_STRING_H)
CHECK_INCLUDE_FILES(strings.h   HAVE_STRINGS_H)

# others
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/config.h.cmake ${CMAKE_BINARY_DIR}/config.h)
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/epm-header.in  ${CMAKE_BINARY_DIR}/epm-header)
SET(PERL ${PERL_EXECUTABLE})
INCLUDE_DIRECTORIES( ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR} )


ADD_EXECUTABLE(add_labels   add_labels.c minc_labels.c)
ADD_EXECUTABLE(apply_sphere_transform   apply_sphere_transform.c)
ADD_EXECUTABLE(autocrop_volume   autocrop_volume.c)
ADD_EXECUTABLE(average_voxels   average_voxels.c)
ADD_EXECUTABLE(blur_surface   blur_surface.c)
ADD_EXECUTABLE(box_filter_volume_nd   box_filter_volume_nd.c)
ADD_EXECUTABLE(box_filter_volume   box_filter_volume.c)
ADD_EXECUTABLE(chamfer_volume   chamfer_volume.c)
ADD_EXECUTABLE(chop_tags   chop_tags.c)
ADD_EXECUTABLE(clamp_volume   clamp_volume.c)
ADD_EXECUTABLE(classify_sulcus   classify_sulcus.c)
ADD_EXECUTABLE(clean_surface_labels  clean_surface_labels.c)
ADD_EXECUTABLE(clip_tags   clip_tags.c)
ADD_EXECUTABLE(close_surface   close_surface.c)
ADD_EXECUTABLE(cluster_volume   cluster_volume.c)
ADD_EXECUTABLE(coalesce_lines   coalesce_lines.c)
ADD_EXECUTABLE(compare_left_right_groups   compare_left_right_groups.c)
ADD_EXECUTABLE(compare_left_right   compare_left_right.c)
ADD_EXECUTABLE(compare_lengths   compare_lengths.c)
ADD_EXECUTABLE(composite_images   composite_images.c)
ADD_EXECUTABLE(composite_minc_images   composite_minc_images.c)
ADD_EXECUTABLE(composite_volumes   composite_volumes.c)
ADD_EXECUTABLE(compute_bounding_view   compute_bounding_view.c)
ADD_EXECUTABLE(compute_resels   compute_resels.c)
ADD_EXECUTABLE(concat_images   concat_images.c)
ADD_EXECUTABLE(contour_slice   contour_slice.c)
ADD_EXECUTABLE(convex_hull  convex_hull.c)
ADD_EXECUTABLE(count_thresholded_volume   count_thresholded_volume.c)
ADD_EXECUTABLE(create_2d_sheet   create_2d_sheet.c)
ADD_EXECUTABLE(create_2d_surface   create_2d_surface.c)
ADD_EXECUTABLE(create_box   create_box.c)
ADD_EXECUTABLE(create_four_volumes   create_four_volumes.c)
ADD_EXECUTABLE(create_label_map   create_label_map.c)
ADD_EXECUTABLE(create_landmark_full_volume   create_landmark_full_volume.c get_filenames.c)
ADD_EXECUTABLE(create_mahalanobis   create_mahalanobis.c)
ADD_EXECUTABLE(create_surface_interpolation_lsq   create_surface_interpolation_lsq.c)
ADD_EXECUTABLE(create_warping_points   create_warping_points.c)
ADD_EXECUTABLE(diff_mahalanobis   diff_mahalanobis.c)
ADD_EXECUTABLE(dilate_volume_completely   dilate_volume_completely.c)
ADD_EXECUTABLE(dilate_volume   dilate_volume.c)
ADD_EXECUTABLE(dim_image   dim_image.c)
ADD_EXECUTABLE(dump_deformation_distances   dump_deformation_distances.c)
ADD_EXECUTABLE(dump_points_to_tag_file   dump_points_to_tag_file.c)
ADD_EXECUTABLE(dump_rms   dump_rms.c)
ADD_EXECUTABLE(dump_transform   dump_transform.c)
ADD_EXECUTABLE(dump_uv   dump_uv.c)
ADD_EXECUTABLE(evaluate   evaluate.c)
ADD_EXECUTABLE(extract_largest_line   extract_largest_line.c)
ADD_EXECUTABLE(extract_tag_slice   extract_tag_slice.c)
ADD_EXECUTABLE(fill_sulci   fill_sulci.c)
ADD_EXECUTABLE(find_buried_surface   find_buried_surface.c)
ADD_EXECUTABLE(find_image_bounding_box   find_image_bounding_box.c)
ADD_EXECUTABLE(find_peaks  find_peaks.c)
ADD_EXECUTABLE(find_surface_distances   find_surface_distances.c search_utils.c find_in_direction.c model_objects.c intersect_voxel.c deform_line.c models.c)
ADD_EXECUTABLE(find_tag_outliers   find_tag_outliers.c)
ADD_EXECUTABLE(find_vertex   find_vertex.c)
ADD_EXECUTABLE(find_volume_centroid   find_volume_centroid.c)
ADD_EXECUTABLE(fit_3d   fit_3d.c find_in_direction.c model_objects.c intersect_voxel.c deform_line.c models.c search_utils.c)
ADD_EXECUTABLE(fit_curve2   fit_curve2.c  conjugate_min.c conjugate_grad.c line_minimization.c)
ADD_EXECUTABLE(fit_curve   fit_curve.c)
ADD_EXECUTABLE(flatten_polygons   flatten_polygons.c)
ADD_EXECUTABLE(flatten_sheet3   flatten_sheet3.c)
ADD_EXECUTABLE(flatten_sheet   flatten_sheet.c)
ADD_EXECUTABLE(flatten_to_sphere2   flatten_to_sphere2.c)
ADD_EXECUTABLE(flatten_to_sphere   flatten_to_sphere.c)
ADD_EXECUTABLE(flip_tags   flip_tags.c)
ADD_EXECUTABLE(flip_volume   flip_volume.c)
ADD_EXECUTABLE(f_prob   f_prob.c)
ADD_EXECUTABLE(gaussian_blur_peaks   gaussian_blur_peaks.c)
ADD_EXECUTABLE(get_tic   get_tic.c)
ADD_EXECUTABLE(group_diff   group_diff.c)
ADD_EXECUTABLE(histogram_volume   histogram_volume.c)
ADD_EXECUTABLE(intensity_statistics   intensity_statistics.c)
ADD_EXECUTABLE(interpolate_tags   interpolate_tags.c)
ADD_EXECUTABLE(labels_to_rgb   labels_to_rgb.c)
ADD_EXECUTABLE(label_sulci   label_sulci.c)
ADD_EXECUTABLE(lookup_labels   lookup_labels.c minc_labels.c)
ADD_EXECUTABLE(make_diff_volume   make_diff_volume.c)
ADD_EXECUTABLE(make_geodesic_volume   make_geodesic_volume.c)
ADD_EXECUTABLE(make_gradient_volume   make_gradient_volume.c)
ADD_EXECUTABLE(make_grid_lines   make_grid_lines.c)
ADD_EXECUTABLE(make_line_links   make_line_links.c)
ADD_EXECUTABLE(make_slice   make_slice.c)
ADD_EXECUTABLE(make_sphere_transform   make_sphere_transform.c)
ADD_EXECUTABLE(make_surface_bitlist   make_surface_bitlist.c)
ADD_EXECUTABLE(map_colours_to_sphere   map_colours_to_sphere.c)
ADD_EXECUTABLE(map_sheets   map_sheets.c)
ADD_EXECUTABLE(map_surface_to_sheet   map_surface_to_sheet.c)
ADD_EXECUTABLE(marching_cubes  marching_cubes.c)
ADD_EXECUTABLE(mask_values   mask_values.c)
ADD_EXECUTABLE(mask_volume   mask_volume.c)
ADD_EXECUTABLE(match_tags  match_tags.c)
ADD_EXECUTABLE(minc_to_rgb   minc_to_rgb.c)
ADD_EXECUTABLE(mincdefrag  mincdefrag.cc)
ADD_EXECUTABLE(mincmask  mincmask.c)
ADD_EXECUTABLE(mincskel  mincskel.cc)
ADD_EXECUTABLE(minctotag   minctotag.c)
ADD_EXECUTABLE(normalize_pet  normalize_pet.c)
ADD_EXECUTABLE(place_images   place_images.c)
ADD_EXECUTABLE(plane_polygon_intersect   plane_polygon_intersect.c)
ADD_EXECUTABLE(preprocess_segmentation   preprocess_segmentation.c)
ADD_EXECUTABLE(print_2d_coords   print_2d_coords.c)
ADD_EXECUTABLE(print_all_label_bounding_boxes   print_all_label_bounding_boxes.c)
ADD_EXECUTABLE(print_all_labels   print_all_labels.c)
ADD_EXECUTABLE(print_axis_angles   print_axis_angles.c)
ADD_EXECUTABLE(print_volume_value   print_volume_value.c)
ADD_EXECUTABLE(print_world_value   print_world_value.c)
ADD_EXECUTABLE(print_world_values   print_world_values.c)
ADD_EXECUTABLE(random_warp   random_warp.c)
ADD_EXECUTABLE(reparameterize_line   reparameterize_line.c)
ADD_EXECUTABLE(rgb_to_minc   rgb_to_minc.c)
ADD_EXECUTABLE(scale_minc_image   scale_minc_image.c)
ADD_EXECUTABLE(scan_lines_to_polygons   scan_lines_to_polygons.c)
ADD_EXECUTABLE(scan_object_to_volume   scan_object_to_volume.c)
ADD_EXECUTABLE(segment_probabilities   segment_probabilities.c)
ADD_EXECUTABLE(spherical_resample   spherical_resample.c)
ADD_EXECUTABLE(stats_tag_file   stats_tag_file.c)
ADD_EXECUTABLE(subsample_volume   subsample_volume.c)
ADD_EXECUTABLE(surface_mask2   surface_mask2.c)
ADD_EXECUTABLE(surface_mask   surface_mask.c)
ADD_EXECUTABLE(tags_to_spheres   tags_to_spheres.c)
ADD_EXECUTABLE(tagtominc   tagtominc.c)
ADD_EXECUTABLE(tag_volume   tag_volume.c)
ADD_EXECUTABLE(threshold_volume   threshold_volume.c)
ADD_EXECUTABLE(transform_objects   transform_objects.c)
ADD_EXECUTABLE(transform_tags   transform_tags.c)
ADD_EXECUTABLE(transform_volume   transform_volume.c)
ADD_EXECUTABLE(trimesh_resample   trimesh_resample.c tri_mesh.c)
ADD_EXECUTABLE(trimesh_set_points   trimesh_set_points.c tri_mesh.c)
ADD_EXECUTABLE(trimesh_to_polygons   trimesh_to_polygons.c tri_mesh.c)
ADD_EXECUTABLE(two_surface_resample   two_surface_resample.c)
ADD_EXECUTABLE(volume_object_evaluate  volume_object_evaluate.c)
  

MACRO(PREPARE_SCRIPT input output)
  # minc 1 version
  CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/${input} ${CMAKE_CURRENT_BINARY_DIR}/${output} @ONLY)
  
  INSTALL(PROGRAMS
    ${CMAKE_CURRENT_BINARY_DIR}/${output}
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ  WORLD_EXECUTE WORLD_READ
    DESTINATION bin )
ENDMACRO(PREPARE_SCRIPT)

PREPARE_SCRIPT(nlfit_smr.in nlfit_smr)
PREPARE_SCRIPT(mritotal_suppress.in mritotal_suppress)
PREPARE_SCRIPT(smooth_mask.in smooth_mask)
PREPARE_SCRIPT(compute_icbm_vols.in compute_icbm_vols)
PREPARE_SCRIPT(multispectral_stx_registration.in multispectral_stx_registration)
PREPARE_SCRIPT(sphere_resample_obj.in sphere_resample_obj)
PREPARE_SCRIPT(suppress_fat.in suppress_fat)
PREPARE_SCRIPT(regional_thickness.in regional_thickness)
PREPARE_SCRIPT(remap_to_lobes.in remap_to_lobes)


INSTALL(TARGETS 
  add_labels 
  apply_sphere_transform 
  autocrop_volume 
  average_voxels 
  blur_surface 
  box_filter_volume 
  box_filter_volume_nd 
  chamfer_volume 
  chop_tags 
  clamp_volume 
  classify_sulcus 
  clean_surface_labels 
  clip_tags 
  close_surface 
  cluster_volume 
  coalesce_lines 
  compare_left_right 
  compare_left_right_groups 
  compare_lengths 
  composite_images 
  composite_minc_images 
  composite_volumes 
  compute_bounding_view 
  compute_resels 
  concat_images 
  contour_slice 
  convex_hull 
  count_thresholded_volume 
  create_2d_sheet 
  create_2d_surface 
  create_box 
  create_four_volumes 
  create_label_map 
  create_landmark_full_volume 
  create_mahalanobis 
  create_surface_interpolation_lsq 
  create_warping_points 
  diff_mahalanobis 
  dilate_volume 
  dilate_volume_completely 
  dim_image 
  dump_deformation_distances 
  dump_points_to_tag_file 
  dump_rms 
  dump_transform 
  dump_uv 
  evaluate 
  extract_tag_slice 
  extract_largest_line 
  fill_sulci 
  find_buried_surface 
  find_image_bounding_box 
  find_peaks 
  find_surface_distances 
  find_tag_outliers 
  find_vertex 
  find_volume_centroid 
  fit_curve 
  fit_curve2 
  fit_3d 
  flatten_polygons 
  flatten_sheet 
  flatten_sheet3 
  flatten_to_sphere 
  flatten_to_sphere2 
  flip_tags 
  flip_volume 
  f_prob 
  gaussian_blur_peaks 
  get_tic 
  group_diff 
  histogram_volume 
  intensity_statistics 
  interpolate_tags 
  label_sulci 
  labels_to_rgb 
  lookup_labels 
  make_diff_volume 
  make_geodesic_volume 
  make_gradient_volume 
  make_grid_lines 
  make_line_links 
  make_slice 
  make_surface_bitlist 
  make_sphere_transform 
  map_colours_to_sphere 
  map_surface_to_sheet 
  map_sheets 
  marching_cubes 
  mask_values 
  mask_volume 
  match_tags 
  mincdefrag 
  mincmask 
  mincskel 
  minctotag 
  minc_to_rgb 
  normalize_pet 
  place_images 
  plane_polygon_intersect 
  preprocess_segmentation 
  print_2d_coords 
  print_all_labels 
  print_all_label_bounding_boxes 
  print_axis_angles 
  print_volume_value 
  print_world_value 
  print_world_values 
  random_warp 
  reparameterize_line 
  rgb_to_minc 
  scale_minc_image 
  scan_lines_to_polygons 
  scan_object_to_volume 
  segment_probabilities 
  spherical_resample 
  stats_tag_file 
  subsample_volume 
  surface_mask 
  surface_mask2 
  tag_volume 
  tagtominc 
  threshold_volume 
  trimesh_resample 
  trimesh_set_points 
  trimesh_to_polygons 
  tags_to_spheres 
  transform_volume 
  transform_tags 
  transform_objects 
  two_surface_resample 
  volume_object_evaluate
  DESTINATION bin)

#TODO install models 

# testing
# IF(BUILD_TESTING)
#   #ADD_SUBDIRECTORY( testing )
# ENDIF(BUILD_TESTING)
