OPT = $(OPT_O)

include $(SRC_DIRECTORY)/BIC_PL/Makefile.include
include $(SRC_DIRECTORY)/David/Makefile.include


INCLUDE = -I. $(BIC_PL_INCLUDE) $(DLIB_INCLUDE) 
LIBS =  $(DLIB_LIBS) $(BIC_PL_LIBS)
LIBS =   $(BIC_PL_LIBS)
LIBS-O3 =  $(DLIB_LIBS-O3) $(BIC_PL_LIBS-O3)
LINT_LIBS = $(DLIB_LINT_LIBS) $(BIC_PL_LINT_LIBS)

roidis_to_display: roidis_to_display.o roidis_to_display.ln
	$(CC) $(LFLAGS) roidis_to_display.o $(LIBS) -o $@
	$(LINT) -x -u $(LINTFLAGS) roidis_to_display.ln $(LINT_LIBS)
                    

###########################################################################

dump_mesh: dump_mesh.o
	$(CC) $(LFLAGS) dump_mesh.o $(LIBS)  -o $@

lint_dump_mesh: dump_mesh.ln
	$(LINT) -x -u $(LINTFLAGS) dump_mesh.ln $(LINT_LIBS)

###########################################################################

make_edges: make_edges.o
	$(CC) $(LFLAGS) make_edges.o $(LIBS) -o $@

lint_make_edges: make_edges.ln
	$(LINT) -x -u $(LINTFLAGS) make_edges.ln $(LINT_LIBS)

###########################################################################

make_edge_points: make_edge_points.o
	$(CC) $(LFLAGS) make_edge_points.o $(LIBS) -o $@

lint_make_edge_points: make_edge_points.ln
	$(LINT) -x -u $(LINTFLAGS) make_edge_points.ln $(LINT_LIBS)

###########################################################################

make_gradient_volume: make_gradient_volume.o
	$(CC) $(LFLAGS) make_gradient_volume.o $(LIBS) -o $@

lint_make_gradient_volume: make_gradient_volume.ln
	$(LINT) -x -u $(LINTFLAGS) make_gradient_volume.ln $(LINT_LIBS)

###########################################################################

make_gradient_arrows: make_gradient_arrows.o
	$(CC) $(LFLAGS) make_gradient_arrows.o $(LIBS) -o $@

lint_make_gradient_arrows: make_gradient_arrows.ln
	$(LINT) -x -u $(LINTFLAGS) make_gradient_arrows.ln $(LINT_LIBS)

###########################################################################

zoom_gradient: zoom_gradient.o
	$(CC) $(LFLAGS) zoom_gradient.o $(LIBS) -o $@

lint_zoom_gradient: zoom_gradient.ln
	$(LINT) -x -u $(LINTFLAGS) zoom_gradient.ln $(LINT_LIBS)

###########################################################################

flip_activity: flip_activity.o
	$(CC) $(LFLAGS) flip_activity.o $(LIBS) -o $@

lint_flip_activity: flip_activity.ln
	$(LINT) -x -u $(LINTFLAGS) flip_activity.ln $(LINT_LIBS)

###########################################################################

create_sphere: create_sphere.o
	$(CC) $(LFLAGS) create_sphere.o $(LIBS) -o $@

lint_create_sphere: create_sphere.ln
	$(LINT) -x -u $(LINTFLAGS) create_sphere.ln $(LINT_LIBS)

###########################################################################

create_ellipse: create_ellipse.o
	$(CC) $(LFLAGS) create_ellipse.o $(LIBS) -o $@

lint_create_ellipse: create_ellipse.ln
	$(LINT) -x -u $(LINTFLAGS) create_ellipse.ln $(LINT_LIBS)

###########################################################################

create_tetra_test: create_tetra.o sp.o poly_neighs.o
	$(CC) $(LFLAGS) create_tetra.o sp.o poly_neighs.o $(LIBS) -o $@

create_tetra: create_tetra.o
	$(CC) $(LFLAGS) create_tetra.o $(LIBS) -o $@

lint_create_tetra: create_tetra.ln
	$(LINT) -x -u $(LINTFLAGS) create_tetra.ln $(LINT_LIBS)

###########################################################################

subdivide_polygons_test: subdivide_polygons.o poly_neighs.o sp.o
	$(CC) $(LFLAGS) subdivide_polygons.o poly_neighs.o sp.o $(LIBS) -o $@

subdivide_polygons: subdivide_polygons.o
	$(CC) $(LFLAGS) subdivide_polygons.o $(LIBS) -o $@

lint_subdivide_polygons: subdivide_polygons.ln
	$(LINT) -x -u $(LINTFLAGS) subdivide_polygons.ln $(LINT_LIBS)

###########################################################################

make_normals: read_write.o make_normals.o
	$(CC) $(LFLAGS) read_write.o make_normals.o $(LIBS) -o $@

lint_make_normals: read_write.ln make_normals.ln
	$(LINT) -x -u $(LINTFLAGS) read_write.ln make_normals.ln $(LINT_LIBS)

###########################################################################

smooth_normals: smooth_normals.o
	$(CC) $(LFLAGS) smooth_normals.o $(LIBS) -o $@

lint_smooth_normals: smooth_normals.ln
	$(LINT) -x -u $(LINTFLAGS) smooth_normals.ln $(LINT_LIBS)

###########################################################################

half_polygons: half_polygons.o
	$(CC) $(LFLAGS) half_polygons.o $(LIBS) -o $@

lint_half_polygons: half_polygons.ln
	$(LINT) -x -u $(LINTFLAGS) half_polygons.ln $(LINT_LIBS)

###########################################################################

deform_surface: deform_surface.o
	$(CC) $(LFLAGS) deform_surface.o $(LIBS) -o $@

deform_surface-O3: deform_surface.u
	$(CC) -O3 $(LFLAGS) deform_surface.u $(LIBS-O3) -o $@

lint_deform_surface: deform_surface.ln
	$(LINT) -x -u $(LINTFLAGS) deform_surface.ln $(LINT_LIBS)

###########################################################################

transform_objects: transform_objects.o
	$(CC) $(LFLAGS) transform_objects.o $(LIBS) -o $@

lint_transform_objects: transform_objects.ln
	$(LINT) -x -u $(LINTFLAGS) transform_objects.ln $(LINT_LIBS) $(MODULES)/lint/llib-lgeometry.ln

###########################################################################

rotate_objects: rotate_objects.o
	$(CC) $(LFLAGS) rotate_objects.o $(LIBS) -o $@

lint_rotate_objects: rotate_objects.ln
	$(LINT) -x -u $(LINTFLAGS) rotate_objects.ln $(LINT_LIBS)

###########################################################################

transform_to_world: transform_to_world.o
	$(CC) $(LFLAGS) transform_to_world.o \
                  $(MODULES)/libgeometry.a $(LIBS) -o $@

lint_transform_to_world: transform_to_world.ln
	$(LINT) -x -u $(LINTFLAGS) transform_to_world.ln $(LINT_LIBS) $(MODULES)/lint/llib-lgeometry.ln

###########################################################################

evaluate: evaluate.o
	$(CC) $(LFLAGS) evaluate.o $(LIBS) -o $@

lint_evaluate: evaluate.ln
	$(LINT) -x -u $(LINTFLAGS) evaluate.ln $(LINT_LIBS)

###########################################################################

voxelate_landmarks: voxelate_landmarks.o
	$(CC) $(LFLAGS) voxelate_landmarks.o $(LIBS) -o $@

lint_voxelate_landmarks: voxelate_landmarks.ln
	$(LINT) -x -u $(LINTFLAGS) voxelate_landmarks.ln \
            $(LINT_LIBS)

###########################################################################

fit_line: fit_line.o
	$(CC) $(LFLAGS) fit_line.o $(LIBS) -o $@

lint_fit_line: fit_line.ln
	$(LINT) -x -u $(LINTFLAGS) fit_line.ln $(LINT_LIBS)

###########################################################################

copy_colours: copy_colours.o
	$(CC) $(LFLAGS) copy_colours.o $(LIBS) -o $@

lint_copy_colours: copy_colours.ln
	$(LINT) -x -u $(LINTFLAGS) copy_colours.ln $(LINT_LIBS)

###########################################################################

ascii_binary: ascii_binary.o
	$(CC) $(LFLAGS) ascii_binary.o $(LIBS) -o $@

ascii_binary-O3: ascii_binary.u
	$(CC) -O3 $(LFLAGS) ascii_binary.u $(LIBS-O3) -o $@

lint_ascii_binary: ascii_binary.ln
	$(LINT) -x -u $(LINTFLAGS) ascii_binary.ln $(LINT_LIBS)

###########################################################################

make_labels: make_labels.o
	$(CC) $(LFLAGS) make_labels.o $(LIBS) -o $@

lint_make_labels: make_labels.ln
	$(LINT) -x -u $(LINTFLAGS) make_labels.ln $(LINT_LIBS)

###########################################################################

examine_polygons: examine_polygons.o
	$(CC) $(LFLAGS) examine_polygons.o $(LIBS) -o $@

lint_examine_polygons: examine_polygons.ln
	$(LINT) -x -u $(LINTFLAGS) examine_polygons.ln  \
                     $(LINT_LIBS)


###########################################################################

create_landmark_volume: create_landmark_volume.o
	$(CC) $(LFLAGS) create_landmark_volume.o $(LIBS) -o $@

lint_create_landmark_volume: create_landmark_volume.ln
	$(LINT) -x -u $(LINTFLAGS) create_landmark_volume.ln  \
                     $(LINT_LIBS)

###########################################################################

create_landmark_full_volume: create_landmark_full_volume.o get_filenames.o
	$(CC) $(LFLAGS) create_landmark_full_volume.o get_filenames.o $(LIBS) -o $@

lint_create_landmark_full_volume: create_landmark_full_volume.ln get_filenames.ln
	$(LINT) -x -u $(LINTFLAGS) create_landmark_full_volume.ln get_filenames.ln  \
                     $(LINT_LIBS)

###########################################################################

colour_curvature: colour_curvature.o
	$(CC) $(LFLAGS) colour_curvature.o $(LIBS) -o $@

lint_colour_curvature: colour_curvature.ln
	$(LINT) -x -u $(LINTFLAGS) colour_curvature.ln \
          $(MODULES)/lint/llib-lgeometry.ln \
          $(LINT_LIBS)


###########################################################################

extract_points: extract_points.o
	$(CC) $(LFLAGS) extract_points.o $(LIBS) -o $@

lint_extract_points: extract_points.ln
	$(LINT) -x -u $(LINTFLAGS) extract_points.ln $(LINT_LIBS)

###########################################################################

dilate_volume: dilate_volume.o
	$(CC) $(LFLAGS) dilate_volume.o $(LIBS) -o $@

lint_dilate_volume: dilate_volume.ln
	$(LINT) -x -u $(LINTFLAGS) dilate_volume.ln $(LINT_LIBS)


###########################################################################

left_minus_right: left_minus_right.o
	$(CC) $(LFLAGS) left_minus_right.o $(LIBS) -o $@

lint_left_minus_right: left_minus_right.ln
	$(LINT) -x -u $(LINTFLAGS) left_minus_right.ln $(LINT_LIBS)

###########################################################################

count_tags: count_tags.o
	$(CC) $(LFLAGS) count_tags.o $(LIBS) -o $@

lint_count_tags: count_tags.ln
	$(LINT) -x -u $(LINTFLAGS) count_tags.ln $(LINT_LIBS)

###########################################################################

count_region: count_region.o
	$(CC) $(LFLAGS) count_region.o $(LIBS) -o $@

lint_count_region: count_region.ln
	$(LINT) -x -u $(LINTFLAGS) count_region.ln $(LINT_LIBS)


###########################################################################

create_volume: create_volume.o
	$(CC) $(LFLAGS) create_volume.o $(LIBS) -o $@

lint_create_volume: create_volume.ln
	$(LINT) -x -u $(LINTFLAGS) create_volume.ln $(LINT_LIBS)


###########################################################################

transform_tags: transform_tags.o
	$(CC) $(LFLAGS) transform_tags.o $(LIBS) -o $@

lint_transform_tags: transform_tags.ln
	$(LINT) -x -u $(LINTFLAGS) transform_tags.ln $(LINT_LIBS)


###########################################################################

tag_statistics: tag_statistics.o
	$(CC) $(LFLAGS) tag_statistics.o  $(LIBS) -o $@

lint_tag_statistics: tag_statistics.ln
	$(LINT) -x -u $(LINTFLAGS) tag_statistics.ln  $(LINT_LIBS)


###########################################################################

remove_singles: remove_singles.o
	$(CC) $(LFLAGS) remove_singles.o $(LIBS) -o $@

lint_remove_singles: remove_singles.ln
	$(LINT) -x -u $(LINTFLAGS) remove_singles.ln $(LINT_LIBS)


###########################################################################

frequencies: frequencies.o
	$(CC) $(LFLAGS) frequencies.o $(LIBS) -o $@

lint_frequencies: frequencies.ln
	$(LINT) -x -u $(LINTFLAGS) frequencies.ln $(LINT_LIBS)


###########################################################################

matrix1: matrix1.o
	$(CC) $(LFLAGS) matrix1.o $(LIBS) -o $@

lint_matrix1: matrix1.ln
	$(LINT) -x -u $(LINTFLAGS) matrix1.ln $(LINT_LIBS)


###########################################################################

check_tags: check_tags.o
	$(CC) $(LFLAGS) check_tags.o $(LIBS) -o $@

lint_check_tags: check_tags.ln
	$(LINT) -x -u $(LINTFLAGS) check_tags.ln $(LINT_LIBS)


###########################################################################

find_interruptions: find_interruptions.o
	$(CC) $(LFLAGS) find_interruptions.o $(LIBS) -o $@

lint_find_interruptions: find_interruptions.ln
	$(LINT) -x -u $(LINTFLAGS) find_interruptions.ln $(LINT_LIBS)


###########################################################################

surface_mask: surface_mask.o
	$(CC) $(LFLAGS) surface_mask.o $(LIBS) -o $@

surface_mask-O3: surface_mask.u
	$(CC) -O3 $(LFLAGS) surface_mask.u $(LIBS-O3) -o $@

surface_mask2: surface_mask2.o
	$(CC) $(LFLAGS) surface_mask2.o $(LIBS) -o $@

surface_mask2-O3: surface_mask2.u
	$(CC) -O3 $(LFLAGS) surface_mask2.u $(LIBS-O3) -o $@

lint_surface_mask: surface_mask.ln
	$(LINT) -x -u $(LINTFLAGS) surface_mask.ln $(LINT_LIBS)


###########################################################################

tag_mask: tag_mask.o
	$(CC) $(LFLAGS) tag_mask.o $(LIBS) -o $@

lint_tag_mask: tag_mask.ln
	$(LINT) -x -u $(LINTFLAGS) tag_mask.ln $(LINT_LIBS)

###########################################################################

add_surfaces: add_surfaces.o
	$(CC) $(LFLAGS) add_surfaces.o $(LIBS) -o $@

lint_add_surfaces: add_surfaces.ln
	$(LINT) -x -u $(LINTFLAGS) add_surfaces.ln $(LINT_LIBS)


###########################################################################

remove_tags: remove_tags.o
	$(CC) $(LFLAGS) remove_tags.o $(LIBS) -o $@

lint_remove_tags: remove_tags.ln
	$(LINT) -x -u $(LINTFLAGS) remove_tags.ln $(LINT_LIBS)


###########################################################################

diff_points: diff_points.o
	$(CC) $(LFLAGS) diff_points.o $(LIBS) -o $@

lint_diff_points: diff_points.ln
	$(LINT) -x -u $(LINTFLAGS) diff_points.ln $(LINT_LIBS)


###########################################################################

stats_tag_file: stats_tag_file.o
	$(CC) $(LFLAGS) stats_tag_file.o $(LIBS) -o $@

lint_stats_tag_file: stats_tag_file.ln
	$(LINT) -x -u $(LINTFLAGS) stats_tag_file.ln  $(LINT_LIBS)

###########################################################################

make_slice_volume: make_slice_volume.o
	$(CC) $(LFLAGS) make_slice_volume.o $(LIBS) -o $@

lint_make_slice_volume: make_slice_volume.ln
	$(LINT) -x -u $(LINTFLAGS) make_slice_volume.ln $(LINT_LIBS)

###########################################################################

create_ellipsoids: create_ellipsoids.o
	$(CC) $(LFLAGS) create_ellipsoids.o \
             $(MODULES)/libgeometry.a \
             $(LIBS) -o $@

lint_create_ellipsoids: create_ellipsoids.ln
	$(LINT) -x -u $(LINTFLAGS) create_ellipsoids.ln $(LINT_LIBS)

###########################################################################

volume_colour: volume_colour.o
	$(CC) $(LFLAGS) volume_colour.o $(LIBS) -o $@

lint_volume_colour: volume_colour.ln
	$(LINT) -x -u $(LINTFLAGS) volume_colour.ln $(LINT_LIBS)


###########################################################################

cards: cards.o
	$(CC) $(LFLAGS) cards.o $(LIBS) -o $@

lint_cards: cards.ln
	$(LINT) -x -u $(LINTFLAGS) cards.ln $(LINT_LIBS)

###########################################################################

lmk_to_tag: lmk_to_tag.o
	$(CC) $(LFLAGS) lmk_to_tag.o $(LIBS) -o $@

lint_lmk_to_tag: lmk_to_tag.ln
	$(LINT) -x -u $(LINTFLAGS) lmk_to_tag.ln $(LINT_LIBS)

###########################################################################

histogram_volume: histogram_volume.o
	$(CC) $(LFLAGS) histogram_volume.o $(LIBS) -o $@

histogram_volume-O3: histogram_volume.u
	$(CC) -O3 $(LFLAGS) histogram_volume.u $(LIBS-O3) -o $@

lint_histogram_volume: histogram_volume.ln
	$(LINT) -x -u $(LINTFLAGS) histogram_volume.ln $(LINT_LIBS)

histogram_volume2: histogram_volume2.o
	$(CC) $(LFLAGS) histogram_volume2.o $(LIBS) -o $@

histogram_volume2-O3: histogram_volume2.u
	$(CC) -O3 $(LFLAGS) histogram_volume2.u $(LIBS-O3) -o $@



###########################################################################

make_slice: make_slice.o
	$(CC) $(LFLAGS) make_slice.o $(LIBS) -o $@

lint_make_slice: make_slice.ln
	$(LINT) -x -u $(LINTFLAGS) make_slice.ln $(LINT_LIBS) 

###########################################################################

test_map: test_map.o
	$(CC) $(LFLAGS) test_map.o $(MODULES)/libgeometry.a $(LIBS) -o $@

lint_test_map: test_map.ln
	$(LINT) -x -u $(LINTFLAGS) test_map.ln \
          $(LINT_LIBS) $(MODULES)/lint/llib-lgeometry.ln

###########################################################################

basic_stats: basic_stats.o
	$(CC) $(LFLAGS) basic_stats.o $(MODULES)/libnumerical.a $(LIBS) \
               -o $@

lint_basic_stats: basic_stats.ln
	$(LINT) -x -u $(LINTFLAGS) basic_stats.ln $(MODULES)/lint/llib-lnumerical.ln \
              $(LINT_LIBS)

###########################################################################

find_vertex: find_vertex.o
	$(CC) $(LFLAGS) find_vertex.o  $(LIBS) -o $@

lint_find_vertex: find_vertex.ln
	$(LINT) -x -u $(LINTFLAGS) find_vertex.ln  $(LINT_LIBS)

###########################################################################

compare_volumes: compare_volumes.o
	$(CC) $(LFLAGS) compare_volumes.o  $(LIBS) -o $@

lint_compare_volumes: compare_volumes.ln
	$(LINT) -x -u $(LINTFLAGS) compare_volumes.ln  $(LINT_LIBS)

###########################################################################

extract_sulci: extract_sulci.o
	$(CC) $(LFLAGS) extract_sulci.o  $(LIBS) -o $@

lint_extract_sulci: extract_sulci.ln
	$(LINT) -x -u $(LINTFLAGS) extract_sulci.ln  $(LINT_LIBS)

###########################################################################

mask_volume: mask_volume.o
	$(CC) $(LFLAGS) mask_volume.o  $(LIBS) -o $@

lint_mask_volume: mask_volume.ln
	$(LINT) -x -u $(LINTFLAGS) mask_volume.ln  $(LINT_LIBS)

###########################################################################

vol2roi: vol2roi.o
	$(CC) $(LFLAGS) vol2roi.o  $(LIBS) -o $@

lint_vol2roi: vol2roi.ln
	$(LINT) -x -u $(LINTFLAGS) vol2roi.ln  $(LINT_LIBS)

###########################################################################

tubify: tubify.o
	$(CC) $(LFLAGS) tubify.o  $(MODULES)/libgeometry.a $(LIBS) -o $@

lint_tubify: tubify.ln
	$(LINT) -x -u $(LINTFLAGS) tubify.ln  $(MODULES)/lint/llib-lgeometry.ln \
             $(LINT_LIBS)

###########################################################################

polygon_map: polygon_map.o
	$(CC) $(LFLAGS) polygon_map.o $(LIBS) -o $@

polygon_map-O3: polygon_map.u
	$(CC) -O3 $(LFLAGS) polygon_map.u $(LIBS-O3) -o $@

lint_polygon_map: polygon_map.ln
	$(LINT) -x -u $(LINTFLAGS) polygon_map.ln $(LINT_LIBS)

###########################################################################

polygons_to_lines: polygons_to_lines.o
	$(CC) $(LFLAGS) polygons_to_lines.o \
                $(LIBS) -o $@

lint_polygons_to_lines: polygons_to_lines.ln
	$(LINT) -x -u $(LINTFLAGS) polygons_to_lines.ln  $(LINT_LIBS)

###########################################################################

plane_polygon_intersect: plane_polygon_intersect.o
	$(CC) $(LFLAGS) plane_polygon_intersect.o \
                $(LIBS) -o $@

lint_plane_polygon_intersect: plane_polygon_intersect.ln
	$(LINT) -x -u $(LINTFLAGS) plane_polygon_intersect.ln  $(LINT_LIBS)

###########################################################################

set_line_width: set_line_width.o
	$(CC) $(LFLAGS) set_line_width.o $(LIBS) -o $@

lint_set_line_width: set_line_width.ln
	$(LINT) -x -u $(LINTFLAGS) set_line_width.ln  $(LINT_LIBS)

###########################################################################

obj_to_rgb: obj_to_rgb.o
	$(CC) $(LFLAGS) obj_to_rgb.o $(LIBS) -o $@

lint_obj_to_rgb: obj_to_rgb.ln
	$(LINT) -x -u $(LINTFLAGS) obj_to_rgb.ln  $(LINT_LIBS)

###########################################################################

set_object_opacity: set_object_opacity.o
	$(CC) $(LFLAGS) set_object_opacity.o $(LIBS) -o $@

lint_set_object_opacity: set_object_opacity.ln
	$(LINT) -x -u $(LINTFLAGS) set_object_opacity.ln  $(LINT_LIBS)


###########################################################################

set_object_colour: set_object_colour.o
	$(CC) $(LFLAGS) set_object_colour.o $(LIBS) -o $@

lint_set_object_colour: set_object_colour.ln
	$(LINT) -x -u $(LINTFLAGS) set_object_colour.ln  $(LINT_LIBS)

###########################################################################

clip_tags: clip_tags.o
	$(CC) $(LFLAGS) clip_tags.o $(LIBS) -o $@

lint_clip_tags: clip_tags.ln
	$(LINT) -x -u $(LINTFLAGS) clip_tags.ln  $(LINT_LIBS)

###########################################################################

tagtominc: tagtominc.o
	$(CC) $(LFLAGS) tagtominc.o $(LIBS) -o $@

lint_tagtominc: tagtominc.ln
	$(LINT) -x -u $(LINTFLAGS) tagtominc.ln  $(LINT_LIBS)

###########################################################################

minctotag: minctotag.o
	$(CC) $(LFLAGS) minctotag.o $(LIBS) -o $@

lint_minctotag: minctotag.ln
	$(LINT) -x -u $(LINTFLAGS) minctotag.ln  $(LINT_LIBS)

###########################################################################

average_voxels: average_voxels.o
	$(CC) $(LFLAGS) average_voxels.o $(LIBS) -o $@

lint_average_voxels: average_voxels.ln
	$(LINT) -x -u $(LINTFLAGS) average_voxels.ln  $(LINT_LIBS)

###########################################################################

change_image_colour: change_image_colour.o
	$(CC) $(LFLAGS) change_image_colour.o $(LIBS) -o $@

lint_change_image_colour: change_image_colour.ln
	$(LINT) -x -u $(LINTFLAGS) change_image_colour.ln  $(LINT_LIBS)

###########################################################################

dump_tal: dump_tal.o
	$(CC) $(LFLAGS) dump_tal.o $(LIBS) -o $@

lint_dump_tal: dump_tal.ln
	$(LINT) -x -u $(LINTFLAGS) dump_tal.ln  $(LINT_LIBS)

###########################################################################

test_arrays: test_arrays.o
	$(CC) $(LFLAGS) test_arrays.o $(LIBS) -o $@

lint_test_arrays: test_arrays.ln
	$(LINT) -x -u $(LINTFLAGS) test_arrays.ln  $(LINT_LIBS)

###########################################################################

clip_polygons: clip_polygons.o
	$(CC) $(LFLAGS) clip_polygons.o $(LIBS) -o $@

lint_clip_polygons: clip_polygons.ln
	$(LINT) -x -u $(LINTFLAGS) clip_polygons.ln $(LINT_LIBS)

###########################################################################

polygon_volume: polygon_volume.o
	$(CC) $(LFLAGS) polygon_volume.o $(LIBS) -o $@

lint_polygon_volume: polygon_volume.ln
	$(LINT) -x -u $(LINTFLAGS) polygon_volume.ln $(LINT_LIBS)

###########################################################################

segment_callosum: segment_callosum.o
	$(CC) $(LFLAGS) segment_callosum.o $(LIBS) -o $@

lint_segment_callosum: segment_callosum.ln
	$(LINT) -x -u $(LINTFLAGS) segment_callosum.ln $(LINT_LIBS)

###########################################################################

add_labels: add_labels.o
	$(CC) $(LFLAGS) add_labels.o $(LIBS) -o $@

lint_add_labels: add_labels.ln
	$(LINT) -x -u $(LINTFLAGS) add_labels.ln $(LINT_LIBS)

###########################################################################

lookup_labels: lookup_labels.o
	$(CC) $(LFLAGS) lookup_labels.o $(LIBS) -o $@

lint_lookup_labels: lookup_labels.ln
	$(LINT) -x -u $(LINTFLAGS) lookup_labels.ln $(LINT_LIBS)

###########################################################################

cluster_volume: cluster_volume.o
	$(CC) $(LFLAGS) cluster_volume.o $(LIBS) -o $@

lint_cluster_volume: cluster_volume.ln
	$(LINT) -x -u $(LINTFLAGS) cluster_volume.ln $(LINT_LIBS)

###########################################################################

test_dynamic: test_dynamic.o
	$(CC) $(LFLAGS) test_dynamic.o $(LIBS) -o $@

lint_test_dynamic: test_dynamic.ln
	$(LINT) -x -u $(LINTFLAGS) test_dynamic.ln $(LINT_LIBS)

###########################################################################

make_backdrop: make_backdrop.o
	$(CC) $(LFLAGS) make_backdrop.o $(LIBS) -o $@

lint_make_backdrop: make_backdrop.ln
	$(LINT) -x -u $(LINTFLAGS) make_backdrop.ln $(LINT_LIBS)

###########################################################################

clamp_volume: clamp_volume.o
	$(CC) $(LFLAGS) clamp_volume.o $(LIBS) -o $@

clamp_volume-O3: clamp_volume.u
	$(CC) -O3 $(LFLAGS) clamp_volume.u $(LIBS-O3) -o $@

lint_clamp_volume: clamp_volume.ln
	$(LINT) -x -u $(LINTFLAGS) clamp_volume.ln $(LINT_LIBS)

###########################################################################

prof:
	prof -quit 2000 -pixie dilate_volume -procedures >&! profiling/procedures
	prof -quit 2000 -pixie dilate_volume -heavy >&! profiling/heavy
	prof -quit 2000 -pixie dilate_volume -lines >&! profiling/lines

###########################################################################

flip_volume: flip_volume.o
	$(CC) $(LFLAGS) flip_volume.o $(LIBS) -o $@

lint_flip_volume: flip_volume.ln
	$(LINT) -x -u $(LINTFLAGS) flip_volume.ln $(LINT_LIBS)

###########################################################################

normalize_intensity: normalize_intensity.o
	$(CC) $(LFLAGS) normalize_intensity.o $(LIBS) -o $@

lint_normalize_intensity: normalize_intensity.ln
	$(LINT) -x -u $(LINTFLAGS) normalize_intensity.ln $(LINT_LIBS)

###########################################################################

test_view: test_view.o
	$(CC) $(LFLAGS) test_view.o $(LIBS) -o $@

lint_test_view: test_view.ln
	$(LINT) -x -u $(LINTFLAGS) test_view.ln $(LINT_LIBS)

###########################################################################

transform_volume: transform_volume.o
	$(CC) $(LFLAGS) transform_volume.o $(LIBS) -o $@

transform_volume-O3: transform_volume.u
	$(CC) -O3 $(LFLAGS) transform_volume.u $(LIBS-O3) -o $@

lint_transform_volume: transform_volume.ln
	$(LINT) -x -u $(LINTFLAGS) transform_volume.ln $(LINT_LIBS)

###########################################################################

box_filter_volume: box_filter_volume.o
	$(CC) $(LFLAGS) box_filter_volume.o $(LIBS) -o $@

box_filter_volume-O3: box_filter_volume.u
	$(CC) -O3 $(LFLAGS) box_filter_volume.u $(LIBS-O3) -o $@

lint_box_filter_volume: box_filter_volume.ln
	$(LINT) -x -u $(LINTFLAGS) box_filter_volume.ln $(LINT_LIBS)

prof.box: box_filter_volume.pixie Makefile
	prof -quit 100 -pixie -h -p box_filter_volume >&! $@

###########################################################################

box_filter_volume_nd: box_filter_volume_nd.o
	$(CC) $(LFLAGS) box_filter_volume_nd.o $(LIBS) -o $@

box_filter_volume_nd-O3: box_filter_volume_nd.u
	$(CC) -O3 $(LFLAGS) box_filter_volume_nd.u $(LIBS-O3) -o $@

lint_box_filter_volume_nd: box_filter_volume_nd.ln
	$(LINT) -x -u $(LINTFLAGS) box_filter_volume_nd.ln $(LINT_LIBS)

box_filter_volume_nd.pixie: box_filter_volume_nd
	pixie -o $@ box_filter_volume_nd

prof.box_nd: box_filter_volume_nd.Counts
	prof -E MI_convert_type -quit 100 -pixie -h -p box_filter_volume_nd >&! $@

###########################################################################

smooth_lines: smooth_lines.o
	$(CC) $(LFLAGS) smooth_lines.o $(LIBS) -o $@

lint_smooth_lines: smooth_lines.ln
	$(LINT) -x -u $(LINTFLAGS) smooth_lines.ln $(LINT_LIBS)

###########################################################################

count_thresholded_volume: count_thresholded_volume.o
	$(CC) $(LFLAGS) count_thresholded_volume.o $(LIBS) -o $@

lint_count_thresholded_volume: count_thresholded_volume.ln
	$(LINT) -x -u $(LINTFLAGS) count_thresholded_volume.ln $(LINT_LIBS)

###########################################################################

EXTRA = /nil/david/Release/BIC_PL/libbicpl.a \
        /nil/david/Release/Volume_io/libvolume_io.a

tagtoxfm.old: tagtoxfm.o
	$(CC) $(LFLAGS) tagtoxfm.o $(EXTRA) $(LIBS) -o $@ -lrecipes

tagtoxfm: tagtoxfm.o
	$(CC) $(LFLAGS) tagtoxfm.o $(LIBS) -o $@ -lrecipes

lint_tagtoxfm: tagtoxfm.ln
	$(LINT) -x -u $(LINTFLAGS) tagtoxfm.ln $(LINT_LIBS)

###########################################################################

perspective_solution: perspective_solution.o
	$(CC) $(LFLAGS) perspective_solution.o $(LIBS) -o $@
#	$(CC) $(LFLAGS) perspective_solution.o  -o $@

lint_perspective_solution: perspective_solution.ln
	$(LINT) -x -u $(LINTFLAGS) perspective_solution.ln $(LINT_LIBS)

###########################################################################

subsample_volume: subsample_volume.o
	$(CC) $(LFLAGS) subsample_volume.o $(LIBS) -o $@

lint_subsample_volume: subsample_volume.ln
	$(LINT) -x -u $(LINTFLAGS) subsample_volume.ln $(LINT_LIBS)

###########################################################################

contour_slice: contour_slice.o
	$(CC) $(LFLAGS) contour_slice.o $(LIBS) -o $@

lint_contour_slice: contour_slice.ln
	$(LINT) -x -u $(LINTFLAGS) contour_slice.ln $(LINT_LIBS)

###########################################################################

test_labels: test_labels.o
	$(CC) $(LFLAGS) test_labels.o $(LIBS) -o $@

lint_test_labels: test_labels.ln
	$(LINT) -x -u $(LINTFLAGS) test_labels.ln $(LINT_LIBS)

###########################################################################

flip_tags: flip_tags.o
	$(CC) $(LFLAGS) flip_tags.o $(LIBS) -o $@

lint_flip_tags: flip_tags.ln
	$(LINT) -x -u $(LINTFLAGS) flip_tags.ln $(LINT_LIBS)

###########################################################################

test_tags: test_tags.o
	$(CC) $(LFLAGS) test_tags.o $(LIBS) -o $@

lint_test_tags: test_tags.ln
	$(LINT) -x -u $(LINTFLAGS) test_tags.ln $(LINT_LIBS)

###########################################################################

tag_volume: tag_volume.o
	$(CC) $(LFLAGS) tag_volume.o $(LIBS) -o $@

lint_tag_volume: tag_volume.ln
	$(LINT) -x -u $(LINTFLAGS) tag_volume.ln $(LINT_LIBS)

###########################################################################

find_tag_outliers: find_tag_outliers.o
	$(CC) $(LFLAGS) find_tag_outliers.o $(LIBS) -o $@

lint_find_tag_outliers: find_tag_outliers.ln
	$(LINT) -x -u $(LINTFLAGS) find_tag_outliers.ln $(LINT_LIBS)

###########################################################################

print_volume_value: print_volume_value.o
	$(CC) $(LFLAGS) print_volume_value.o $(LIBS) -o $@

lint_print_volume_value: print_volume_value.ln
	$(LINT) -x -u $(LINTFLAGS) print_volume_value.ln $(LINT_LIBS)

###########################################################################

create_grid: create_grid.o
	$(CC) $(LFLAGS) create_grid.o $(LIBS) -o $@

lint_create_grid: create_grid.ln
	$(LINT) -x -u $(LINTFLAGS) create_grid.ln $(LINT_LIBS)

###########################################################################

convex_hull: convex_hull.o
	$(CC) $(LFLAGS) convex_hull.o $(LIBS) -o $@

lint_convex_hull: convex_hull.ln
	$(LINT) -x -u $(LINTFLAGS) convex_hull.ln $(LINT_LIBS)

###########################################################################

label_inside_surface: label_inside_surface.o
	$(CC) $(LFLAGS) label_inside_surface.o $(LIBS) -o $@

lint_label_inside_surface: label_inside_surface.ln
	$(LINT) -x -u $(LINTFLAGS) label_inside_surface.ln $(LINT_LIBS)

###########################################################################

find_polygon_containing_point: find_polygon_containing_point.o
	$(CC) $(LFLAGS) find_polygon_containing_point.o $(LIBS) -o $@

lint_find_polygon_containing_point: find_polygon_containing_point.ln
	$(LINT) -x -u $(LINTFLAGS) find_polygon_containing_point.ln $(LINT_LIBS)

###########################################################################

close_surface: close_surface.o
	$(CC) $(LFLAGS) close_surface.o $(LIBS) -o $@

lint_close_surface: close_surface.ln
	$(LINT) -x -u $(LINTFLAGS) close_surface.ln $(LINT_LIBS)

###########################################################################

threshold_volume: threshold_volume.o
	$(CC) $(LFLAGS) threshold_volume.o $(LIBS) -o $@

threshold_volume-O3: threshold_volume.u
	$(CC) -O3 $(LFLAGS) threshold_volume.u $(LIBS-O3) -o $@

lint_threshold_volume: threshold_volume.ln
	$(LINT) -x -u $(LINTFLAGS) threshold_volume.ln $(LINT_LIBS)

###########################################################################

dump_slice: dump_slice.o
	$(CC) $(LFLAGS) dump_slice.o $(LIBS) -o $@

lint_dump_slice: dump_slice.ln
	$(LINT) -x -u $(LINTFLAGS) dump_slice.ln $(LINT_LIBS)

###########################################################################

create_four_volumes: create_four_volumes.o
	$(CC) $(LFLAGS) create_four_volumes.o $(LIBS) -o $@

lint_create_four_volumes: create_four_volumes.ln
	$(LINT) -x -u $(LINTFLAGS) create_four_volumes.ln $(LINT_LIBS)

###########################################################################

autocrop_volume: autocrop_volume.o
	$(CC) $(LFLAGS) autocrop_volume.o $(LIBS) -o $@

lint_autocrop_volume: autocrop_volume.ln
	$(LINT) -x -u $(LINTFLAGS) autocrop_volume.ln $(LINT_LIBS)

###########################################################################

CONGLOMERATE_SOURCE = \
                      add_labels.c \
                      add_surfaces.c \
                      apply_sphere_transform.c \
                      autocrop_volume.c \
                      average_objects.c \
                      average_voxels.c \
                      average_surfaces.c \
                      blur_surface.c \
                      box_filter_volume.c \
                      box_filter_volume_nd.c \
                      chamfer_volume.c \
                      check_polygons.c \
                      chop_tags.c \
                      clamp_volume.c \
                      classify_sulcus.c \
                      clip_tags.c \
                      close_surface.c \
                      cluster_volume.c \
                      coalesce_lines.c \
                      colour_object.c \
                      compare_left_right.c \
                      compare_left_right_groups.c \
                      compare_lengths.c \
                      composite_images.c \
                      composite_minc_images.c \
                      composite_volumes.c \
                      compute_bounding_view.c \
                      compute_resels.c \
                      concat_images.c \
                      contour_slice.c \
                      copy_colours.c \
                      count_thresholded_volume.c \
                      create_2d_sheet.c \
                      create_2d_surface.c \
                      create_box.c \
                      create_four_volumes.c \
                      create_grid.c \
                      create_label_map.c \
                      create_landmark_full_volume.c \
                      create_mahalanobis.c \
                      create_rectangle.c \
                      create_surface_interpolation_lsq.c \
                      create_tetra.c \
                      create_warping_points.c \
                      diff_mahalanobis.c \
                      diff_points.c \
                      diff_surfaces.c \
                      dilate_volume.c \
                      dilate_volume_completely.c \
                      dim_image.c \
                      dump_curvatures.c \
                      dump_deformation_distances.c \
                      dump_point_diffs.c \
                      dump_points.c \
                      dump_points_to_tag_file.c \
                      dump_polygons.c \
                      dump_rms.c \
                      dump_transform.c \
                      dump_uv.c \
                      evaluate.c \
                      extract_tag_slice.c \
                      extract_largest_line.c \
                      find_image_bounding_box.c \
                      fill_sulci.c \
                      find_buried_surface.c \
                      find_surface_distances.c \
                      find_tag_outliers.c \
                      find_vertex.c \
                      find_volume_centroid.c \
                      fit_curve.c \
                      fit_curve2.c \
                      fit_3d.c \
                      flatten_polygons.c \
                      flatten_sheet.c \
                      flatten_sheet3.c \
                      flatten_to_sphere.c \
                      flatten_to_sphere2.c \
                      flip_tags.c \
                      flip_volume.c \
                      f_prob.c \
                      gaussian_blur_peaks.c \
                      get_tic.c \
                      group_diff.c \
                      half_polygons.c \
                      histogram_volume.c \
                      intensity_statistics.c \
                      interpolate_tags.c \
                      label_sulci.c \
                      labels_to_rgb.c \
                      lookup_labels.c \
                      make_concentric_surface.c \
                      make_diff_volume.c \
                      make_geodesic_volume.c \
                      make_gradient_volume.c \
                      make_grid_lines.c \
                      make_line_links.c \
                      make_slice.c \
                      make_surface_bitlist.c \
                      make_colour_bar.c \
                      make_sphere_transform.c \
                      manifold_polygons.c \
                      map_colours_to_sphere.c \
                      map_surface_to_sheet.c \
                      map_sheets.c \
                      mask_values.c \
                      mask_volume.c \
                      measure_surface_area.c \
                      merge_polygons.c \
                      minctotag.c \
                      minc_to_rgb.c \
                      perturb_surface.c \
                      place_images.c \
                      plane_polygon_intersect.c \
                      polygon_map.c \
                      polygons_to_lines.c \
                      preprocess_segmentation.c \
                      print_2d_coords.c \
                      print_all_labels.c \
                      print_all_label_bounding_boxes.c \
                      print_axis_angles.c \
                      print_n_polygons.c \
                      print_object_centroid.c \
                      print_object_limits.c \
                      print_volume_value.c \
                      print_world_value.c \
                      random_warp.c \
                      reconstitute_points.c \
                      refine_mesh.c \
                      reparameterize_line.c \
                      rgb_to_minc.c \
                      scale_minc_image.c \
                      scan_lines_to_polygons.c \
                      scan_object_to_volume.c \
                      segment_probabilities.c \
                      separate_polygons.c \
                      set_line_width.c \
                      set_object_colour.c \
                      set_object_opacity.c \
                      set_object_surfprop.c \
                      smooth_lines.c \
                      smooth_normals.c \
                      spherical_resample.c \
                      spline_lines.c \
                      split_polygons.c \
                      stats_tag_file.c \
                      subsample_volume.c \
                      subdivide_polygons.c \
                      subdivide_values.c \
                      surface_mask.c \
                      surface_mask2.c \
                      tag_volume.c \
                      tagtominc.c \
                      threshold_volume.c \
                      triangulate_polygons.c \
                      trimesh_resample.c \
                      trimesh_set_points.c \
                      trimesh_to_polygons.c \
                      ascii_binary.c \
                      tags_to_spheres.c \
                      transform_volume.c \
                      transform_tags.c \
                      transform_objects.c \
                      two_surface_resample.c \
                      volume_object_evaluate.c

include Conglomerate/Makefile.conglomerate

Conglomerate/Makefile.conglomerate:
	Conglomerate/generate_big_main.pl Conglomerate/conglomerate.c \
                                          $(CONGLOMERATE_SOURCE) >& $@

CONG_OBJECTS = Conglomerate/conglomerate.o \
               get_filenames.o \
               conjugate_min.o \
               conjugate_grad.o \
               line_minimization.o \
               $(CONGLOMERATE_OBJECTS)

conglomerate: $(CONG_OBJECTS)
	$(CC) $(LFLAGS) $(CONG_OBJECTS) $(LIBS) -o $@

update:	conglomerate
	-/bin/mv /nil/david/public_bin/conglomerate \
             /nil/david/public_bin/conglomerate.to_delete
	/bin/mv  conglomerate /nil/david/public_bin
	/bin/rm -f /nil/david/public_bin/conglomerate.to_delete

update6: conglomerate
	mv /nil/david/public_bin/IRIX_6/conglomerate \
             /nil/david/public_bin/IRIX_6/conglomerate.to_delete
	mv conglomerate /nil/david/public_bin/IRIX_6
	\rm -f /nil/david/public_bin/IRIX_6/conglomerate.to_delete

update-O3:	conglomerate-O3
	strip conglomerate-O3
	mv /nil/david/public_bin/conglomerate \
             /nil/david/public_bin/conglomerate.to_delete
	mv conglomerate-O3 /nil/david/public_bin/conglomerate
	\rm -f /nil/david/public_bin/conglomerate.to_delete

conglomerate-O3: $(CONG_OBJECTS:.o=.u)
	$(CC) $(LFLAGS) -O3 $(CONG_OBJECTS:.o=.u) $(LIBS-O3) -o $@

lint_conglomerate: $(CONG_OBJECTS:.o=.ln)
	$(LINT) -x -u $(LINTFLAGS) $(CONG_OBJECTS:.o=.ln) $(LINT_LIBS)

###########################################################################

rgb_to_minc: rgb_to_minc.o
	$(CC) $(LFLAGS) rgb_to_minc.o $(LIBS) -o $@

lint_rgb_to_minc: rgb_to_minc.ln
	$(LINT) -x -u $(LINTFLAGS) rgb_to_minc.ln $(LINT_LIBS)

###########################################################################

chop_tags: chop_tags.o
	$(CC) $(LFLAGS) chop_tags.o $(LIBS) -o $@

lint_chop_tags: chop_tags.ln
	$(LINT) -x -u $(LINTFLAGS) chop_tags.ln $(LINT_LIBS)

###########################################################################

intensity_statistics: intensity_statistics.o
	$(CC) $(LFLAGS) intensity_statistics.o $(LIBS) -o $@

intensity_statistics-O3: intensity_statistics.u
	$(CC) $(LFLAGS) -O3 intensity_statistics.u $(LIBS-O3) -o $@

lint_intensity_statistics: intensity_statistics.ln
	$(LINT) -x -u $(LINTFLAGS) intensity_statistics.ln $(LINT_LIBS)

###########################################################################

test_ray_intersect: test_ray_intersect.o
	$(CC) $(LFLAGS) test_ray_intersect.o $(LIBS) -o $@

lint_test_ray_intersect: test_ray_intersect.ln
	$(LINT) -x -u $(LINTFLAGS) test_ray_intersect.ln $(LINT_LIBS)

###########################################################################

triangulate_polygons: triangulate_polygons.o
	$(CC) $(LFLAGS) triangulate_polygons.o $(LIBS) -o $@

lint_triangulate_polygons: triangulate_polygons.ln
	$(LINT) -x -u $(LINTFLAGS) triangulate_polygons.ln $(LINT_LIBS)

###########################################################################

extrude_lines: extrude_lines.o
	$(CC) $(LFLAGS) extrude_lines.o $(LIBS) -o $@

lint_extrude_lines: extrude_lines.ln
	$(LINT) -x -u $(LINTFLAGS) extrude_lines.ln $(LINT_LIBS)

###########################################################################

extend_slice: extend_slice.o
	$(CC) $(LFLAGS) extend_slice.o $(LIBS) -o $@

lint_extend_slice: extend_slice.ln
	$(LINT) -x -u $(LINTFLAGS) extend_slice.ln $(LINT_LIBS)

###########################################################################

change_for_eva: change_for_eva.o
	$(CC) $(LFLAGS) change_for_eva.o $(LIBS) -o $@

lint_change_for_eva: change_for_eva.ln
	$(LINT) -x -u $(LINTFLAGS) change_for_eva.ln $(LINT_LIBS)

###########################################################################

fill_image: fill_image.o
	$(CC) $(LFLAGS) fill_image.o $(LIBS) -o $@

lint_fill_image: fill_image.ln
	$(LINT) -x -u $(LINTFLAGS) fill_image.ln $(LINT_LIBS)

###########################################################################

map_surface_to_sheet: map_surface_to_sheet.o
	$(CC) $(LFLAGS) map_surface_to_sheet.o $(LIBS) -o $@

lint_map_surface_to_sheet: map_surface_to_sheet.ln
	$(LINT) -x -u $(LINTFLAGS) map_surface_to_sheet.ln $(LINT_LIBS)

###########################################################################

map_objects_to_sheet: map_objects_to_sheet.o
	$(CC) $(LFLAGS) map_objects_to_sheet.o $(LIBS) -o $@

lint_map_objects_to_sheet: map_objects_to_sheet.ln
	$(LINT) -x -u $(LINTFLAGS) map_objects_to_sheet.ln $(LINT_LIBS)

###########################################################################

dilate_image: dilate_image.o
	$(CC) $(LFLAGS) dilate_image.o $(LIBS) -o $@

lint_dilate_image: dilate_image.ln
	$(LINT) -x -u $(LINTFLAGS) dilate_image.ln $(LINT_LIBS)

###########################################################################

analyze_grid: analyze_grid.o
	$(CC) $(LFLAGS) analyze_grid.o $(LIBS) -o $@

lint_analyze_grid: analyze_grid.ln
	$(LINT) -x -u $(LINTFLAGS) analyze_grid.ln $(LINT_LIBS)

###########################################################################

transform_labels: transform_labels.o
	$(CC) $(LFLAGS) transform_labels.o $(LIBS) -o $@

lint_transform_labels: transform_labels.ln
	$(LINT) -x -u $(LINTFLAGS) transform_labels.ln $(LINT_LIBS)

###########################################################################

perturb_labels: perturb_labels.o
	$(CC) $(LFLAGS) perturb_labels.o $(LIBS) -o $@

lint_perturb_labels: perturb_labels.ln
	$(LINT) -x -u $(LINTFLAGS) perturb_labels.ln $(LINT_LIBS)

###########################################################################

average_surfaces: average_surfaces.o
	$(CC) $(LFLAGS) average_surfaces.o $(LIBS) -o $@

lint_average_surfaces: average_surfaces.ln
	$(LINT) -x -u $(LINTFLAGS) average_surfaces.ln $(LINT_LIBS)

###########################################################################

colour_object: colour_object.o
	$(CC) $(LFLAGS) colour_object.o $(LIBS) -o $@

lint_colour_object: colour_object.ln
	$(LINT) -x -u $(LINTFLAGS) colour_object.ln $(LINT_LIBS)

###########################################################################

dump_point_diffs: dump_point_diffs.o
	$(CC) $(LFLAGS) dump_point_diffs.o $(LIBS) -o $@

lint_dump_point_diffs: dump_point_diffs.ln
	$(LINT) -x -u $(LINTFLAGS) dump_point_diffs.ln $(LINT_LIBS)

###########################################################################

dump_deformation_distances: dump_deformation_distances.o
	$(CC) $(LFLAGS) dump_deformation_distances.o $(LIBS) -o $@

lint_dump_deformation_distances: dump_deformation_distances.ln
	$(LINT) -x -u $(LINTFLAGS) dump_deformation_distances.ln $(LINT_LIBS)

###########################################################################

make_colour_bar: make_colour_bar.o
	$(CC) $(LFLAGS) make_colour_bar.o $(LIBS) -o $@

lint_make_colour_bar: make_colour_bar.ln
	$(LINT) -x -u $(LINTFLAGS) make_colour_bar.ln $(LINT_LIBS)

###########################################################################

spline_lines: spline_lines.o
	$(CC) $(LFLAGS) spline_lines.o $(LIBS) -o $@

lint_spline_lines: spline_lines.ln
	$(LINT) -x -u $(LINTFLAGS) spline_lines.ln $(LINT_LIBS)

###########################################################################

coalesce_lines: coalesce_lines.o
	$(CC) $(LFLAGS) coalesce_lines.o $(LIBS) -o $@

lint_coalesce_lines: coalesce_lines.ln
	$(LINT) -x -u $(LINTFLAGS) coalesce_lines.ln $(LINT_LIBS)

###########################################################################

create_rectangle: create_rectangle.o
	$(CC) $(LFLAGS) create_rectangle.o $(LIBS) -o $@

lint_create_rectangle: create_rectangle.ln
	$(LINT) -x -u $(LINTFLAGS) create_rectangle.ln $(LINT_LIBS)

###########################################################################

make_concentric_surface: make_concentric_surface.o
	$(CC) $(LFLAGS) make_concentric_surface.o $(LIBS) -o $@

lint_make_concentric_surface: make_concentric_surface.ln
	$(LINT) -x -u $(LINTFLAGS) make_concentric_surface.ln $(LINT_LIBS)

###########################################################################

random_warp: random_warp.o
	$(CC) $(LFLAGS) random_warp.o $(LIBS) -o $@

lint_random_warp: random_warp.ln
	$(LINT) -x -u $(LINTFLAGS) random_warp.ln $(LINT_LIBS)

###########################################################################

create_mahalanobis: create_mahalanobis.o
	$(CC) $(LFLAGS) create_mahalanobis.o $(LIBS) -o $@

lint_create_mahalanobis: create_mahalanobis.ln
	$(LINT) -x -u $(LINTFLAGS) create_mahalanobis.ln $(LINT_LIBS)

###########################################################################

dump_rms: dump_rms.o
	$(CC) $(LFLAGS) dump_rms.o $(LIBS) -o $@

lint_dump_rms: dump_rms.ln
	$(LINT) -x -u $(LINTFLAGS) dump_rms.ln $(LINT_LIBS)

###########################################################################

diff_mahalanobis: diff_mahalanobis.o
	$(CC) $(LFLAGS) diff_mahalanobis.o $(LIBS) -o $@

lint_diff_mahalanobis: diff_mahalanobis.ln
	$(LINT) -x -u $(LINTFLAGS) diff_mahalanobis.ln $(LINT_LIBS)

###########################################################################

dump_points: dump_points.o
	$(CC) $(LFLAGS) dump_points.o $(LIBS) -o $@

lint_dump_points: dump_points.ln
	$(LINT) -x -u $(LINTFLAGS) dump_points.ln $(LINT_LIBS)

###########################################################################

compute_resels: compute_resels.o
	$(CC) $(LFLAGS) compute_resels.o $(LIBS) -o $@

lint_compute_resels: compute_resels.ln
	$(LINT) -x -u $(LINTFLAGS) compute_resels.ln $(LINT_LIBS)

###########################################################################

diff_surfaces: diff_surfaces.o
	$(CC) $(LFLAGS) diff_surfaces.o $(LIBS) -o $@

lint_diff_surfaces: diff_surfaces.ln
	$(LINT) -x -u $(LINTFLAGS) diff_surfaces.ln $(LINT_LIBS)

###########################################################################

f_prob: f_prob.o
	$(CC) $(LFLAGS) f_prob.o $(LIBS) -o $@

lint_f_prob: f_prob.ln
	$(LINT) -x -u $(LINTFLAGS) f_prob.ln $(LINT_LIBS)

###########################################################################

blur_surface: blur_surface.o
	$(CC) $(LFLAGS) blur_surface.o $(LIBS) -o $@

blur_surface-O3: blur_surface.u
	$(CC) -O3 $(LFLAGS) blur_surface.u $(LIBS-O3) -o $@

blur_surface_test: blur_surface.o poly_neighs.o
	$(CC) $(LFLAGS) blur_surface.o poly_neighs.o $(LIBS) -o $@

lint_blur_surface: blur_surface.ln
	$(LINT) -x -u $(LINTFLAGS) blur_surface.ln $(LINT_LIBS)

prof.blur: blur_surface.pixie Makefile
	prof -quit 100 -pixie -h -p blur_surface >&! $@


###########################################################################

perturb_surface: perturb_surface.o
	$(CC) $(LFLAGS) perturb_surface.o $(LIBS) -o $@

lint_perturb_surface: perturb_surface.ln
	$(LINT) -x -u $(LINTFLAGS) perturb_surface.ln $(LINT_LIBS)

###########################################################################

group_diff: group_diff.o
	$(CC) $(LFLAGS) group_diff.o $(LIBS) -o $@

lint_group_diff: group_diff.ln
	$(LINT) -x -u $(LINTFLAGS) group_diff.ln $(LINT_LIBS)

###########################################################################

test_swap: test_swap.o
	$(CC) $(LFLAGS) test_swap.o $(LIBS) -o $@

lint_test_swap: test_swap.ln
	$(LINT) -x -u $(LINTFLAGS) test_swap.ln $(LINT_LIBS)

###########################################################################

get_tic: get_tic.o
	$(CC) $(LFLAGS) get_tic.o $(LIBS) -o $@

lint_get_tic: get_tic.ln
	$(LINT) -x -u $(LINTFLAGS) get_tic.ln $(LINT_LIBS)

###########################################################################

dump_curvatures: dump_curvatures.o
	$(CC) $(LFLAGS) dump_curvatures.o $(LIBS) -o $@

dump_curvatures-O3: dump_curvatures.u
	$(CC) -O3 $(LFLAGS) dump_curvatures.u $(LIBS-O3) -o $@

dump_curvatures.pixie: dump_curvatures
	pixie -o $@ dump_curvatures

prof.dump: dump_curvatures.Counts Makefile
	prof -quit 100 -pixie -h -p dump_curvatures >&! $@


lint_dump_curvatures: dump_curvatures.ln
	$(LINT) -x -u $(LINTFLAGS) dump_curvatures.ln $(LINT_LIBS)

###########################################################################

labels_to_rgb: labels_to_rgb.o
	$(CC) $(LFLAGS) labels_to_rgb.o $(LIBS) -o $@

lint_labels_to_rgb: labels_to_rgb.ln
	$(LINT) -x -u $(LINTFLAGS) labels_to_rgb.ln $(LINT_LIBS)

###########################################################################

film_loop: film_loop.o
	$(CC) $(LFLAGS) film_loop.o $(LIBS) -o $@

lint_film_loop: film_loop.ln
	$(LINT) -x -u $(LINTFLAGS) film_loop.ln $(LINT_LIBS)

###########################################################################

sort_volume: sort_volume.o
	$(CC) $(LFLAGS) sort_volume.o $(LIBS) -o $@

lint_sort_volume: sort_volume.ln
	$(LINT) -x -u $(LINTFLAGS) sort_volume.ln $(LINT_LIBS)

###########################################################################

make_histogram_volume: make_histogram_volume.o
	$(CC) $(LFLAGS) make_histogram_volume.o $(LIBS) -o $@

make_histogram_volume-O3: make_histogram_volume.u
	$(CC) -O3 $(LFLAGS) make_histogram_volume.u $(LIBS-O3) -o $@

lint_make_histogram_volume: make_histogram_volume.ln
	$(LINT) -x -u $(LINTFLAGS) make_histogram_volume.ln $(LINT_LIBS)

###########################################################################

scale_volume: scale_volume.o
	$(CC) $(LFLAGS) scale_volume.o $(LIBS) -o $@

lint_scale_volume: scale_volume.ln
	$(LINT) -x -u $(LINTFLAGS) scale_volume.ln $(LINT_LIBS)

###########################################################################

compare_left_right: compare_left_right.o
	$(CC) $(LFLAGS) compare_left_right.o $(LIBS) -o $@

compare_left_right-O3: compare_left_right.u
	$(CC) -O3 $(LFLAGS) compare_left_right.u $(LIBS-O3) -o $@

lint_compare_left_right: compare_left_right.ln
	$(LINT) -x -u $(LINTFLAGS) compare_left_right.ln $(LINT_LIBS)

###########################################################################

evaluate_cross_correlation: evaluate_cross_correlation.o
	$(CC) $(LFLAGS) evaluate_cross_correlation.o $(LIBS) -o $@

evaluate_cross_correlation-O3: evaluate_cross_correlation.u
	$(CC) -O3 $(LFLAGS) evaluate_cross_correlation.u $(LIBS-O3) -o $@

lint_evaluate_cross_correlation: evaluate_cross_correlation.ln
	$(LINT) -x -u $(LINTFLAGS) evaluate_cross_correlation.ln $(LINT_LIBS)

###########################################################################

flip_talairach_brain: flip_talairach_brain.o
	$(CC) $(LFLAGS) flip_talairach_brain.o $(LIBS) -o $@

flip_talairach_brain-O3: flip_talairach_brain.u
	$(CC) -O3 $(LFLAGS) flip_talairach_brain.u $(LIBS-O3) -o $@

lint_flip_talairach_brain: flip_talairach_brain.ln
	$(LINT) -x -u $(LINTFLAGS) flip_talairach_brain.ln $(LINT_LIBS)

###########################################################################

find_hemi: find_hemi.o
	$(CC) $(LFLAGS) find_hemi.o $(LIBS) -o $@

find_hemi-O3: find_hemi.u
	$(CC) -O3 $(LFLAGS) find_hemi.u $(LIBS-O3) -o $@

lint_find_hemi: find_hemi.ln
	$(LINT) -x -u $(LINTFLAGS) find_hemi.ln $(LINT_LIBS)

###########################################################################

find_volume_centroid: find_volume_centroid.o
	$(CC) $(LFLAGS) find_volume_centroid.o $(LIBS) -o $@

find_volume_centroid-O3: find_volume_centroid.u
	$(CC) -O3 $(LFLAGS) find_volume_centroid.u $(LIBS-O3) -o $@

lint_find_volume_centroid: find_volume_centroid.ln
	$(LINT) -x -u $(LINTFLAGS) find_volume_centroid.ln $(LINT_LIBS)

###########################################################################

dump_points_to_tag_file: dump_points_to_tag_file.o
	$(CC) $(LFLAGS) dump_points_to_tag_file.o $(LIBS) -o $@

dump_points_to_tag_file-O3: dump_points_to_tag_file.u
	$(CC) -O3 $(LFLAGS) dump_points_to_tag_file.u $(LIBS-O3) -o $@

lint_dump_points_to_tag_file: dump_points_to_tag_file.ln
	$(LINT) -x -u $(LINTFLAGS) dump_points_to_tag_file.ln $(LINT_LIBS)

###########################################################################

compare_left_right_groups: compare_left_right_groups.o
	$(CC) $(LFLAGS) compare_left_right_groups.o $(LIBS) -o $@

compare_left_right_groups-O3: compare_left_right_groups.u
	$(CC) -O3 $(LFLAGS) compare_left_right_groups.u $(LIBS-O3) -o $@

lint_compare_left_right_groups: compare_left_right_groups.ln
	$(LINT) -x -u $(LINTFLAGS) compare_left_right_groups.ln $(LINT_LIBS)

###########################################################################

map_colours_to_sphere: map_colours_to_sphere.o
	$(CC) $(LFLAGS) map_colours_to_sphere.o $(LIBS) -o $@

map_colours_to_sphere-O3: map_colours_to_sphere.u
	$(CC) -O3 $(LFLAGS) map_colours_to_sphere.u $(LIBS-O3) -o $@

lint_map_colours_to_sphere: map_colours_to_sphere.ln
	$(LINT) -x -u $(LINTFLAGS) map_colours_to_sphere.ln $(LINT_LIBS)

###########################################################################

create_box: create_box.o
	$(CC) $(LFLAGS) create_box.o $(LIBS) -o $@

create_box-O3: create_box.u
	$(CC) -O3 $(LFLAGS) create_box.u $(LIBS-O3) -o $@

lint_create_box: create_box.ln
	$(LINT) -x -u $(LINTFLAGS) create_box.ln $(LINT_LIBS)

###########################################################################

make_core_volume: make_core_volume.o
	$(CC) $(LFLAGS) make_core_volume.o $(LIBS) -o $@

make_core_volume-O3: make_core_volume.u
	$(CC) -O3 $(LFLAGS) make_core_volume.u $(LIBS-O3) -o $@

lint_make_core_volume: make_core_volume.ln
	$(LINT) -x -u $(LINTFLAGS) make_core_volume.ln $(LINT_LIBS)

###########################################################################

similarity_volume: similarity_volume.o
	$(CC) $(LFLAGS) similarity_volume.o $(LIBS) -o $@

similarity_volume-O3: similarity_volume.u
	$(CC) -O3 $(LFLAGS) similarity_volume.u $(LIBS-O3) -o $@

lint_similarity_volume: similarity_volume.ln
	$(LINT) -x -u $(LINTFLAGS) similarity_volume.ln $(LINT_LIBS)

###########################################################################

print_n_polygons: print_n_polygons.o
	$(CC) $(LFLAGS) print_n_polygons.o $(LIBS) -o $@

print_n_polygons-O3: print_n_polygons.u
	$(CC) -O3 $(LFLAGS) print_n_polygons.u $(LIBS-O3) -o $@

lint_print_n_polygons: print_n_polygons.ln
	$(LINT) -x -u $(LINTFLAGS) print_n_polygons.ln $(LINT_LIBS)

###########################################################################

match_histograms: match_histograms.o
	$(CC) $(LFLAGS) match_histograms.o $(LIBS) -o $@

match_histograms-O3: match_histograms.u
	$(CC) -O3 $(LFLAGS) match_histograms.u $(LIBS-O3) -o $@

lint_match_histograms: match_histograms.ln
	$(LINT) -x -u $(LINTFLAGS) match_histograms.ln $(LINT_LIBS)

###########################################################################

concat_images: concat_images.o
	$(CC) $(LFLAGS) concat_images.o $(LIBS) -o $@

concat_images-O3: concat_images.u
	$(CC) -O3 $(LFLAGS) concat_images.u $(LIBS-O3) -o $@

lint_concat_images: concat_images.ln
	$(LINT) -x -u $(LINTFLAGS) concat_images.ln $(LINT_LIBS)

###########################################################################

split_images: split_images.o
	$(CC) $(LFLAGS) split_images.o $(LIBS) -o $@

split_images-O3: split_images.u
	$(CC) -O3 $(LFLAGS) split_images.u $(LIBS-O3) -o $@

lint_split_images: split_images.ln
	$(LINT) -x -u $(LINTFLAGS) split_images.ln $(LINT_LIBS)

###########################################################################

find_surface_distances: find_surface_distances.o
	$(CC) $(LFLAGS) find_surface_distances.o $(LIBS) -o $@

###########################################################################

convert_rgb: convert_rgb.o
	$(CC) $(LFLAGS) convert_rgb.o $(LIBS) -o $@

###########################################################################

make_geodesic_volume-O3: make_geodesic_volume.u
	$(CC) -O3 $(LFLAGS) make_geodesic_volume.u $(LIBS-O3) -o $@

make_geodesic_volume: make_geodesic_volume.o
	$(CC) $(LFLAGS) make_geodesic_volume.o $(LIBS) -o $@

###########################################################################

make_surface_bitlist-O3: make_surface_bitlist.u
	$(CC) -O3 $(LFLAGS) make_surface_bitlist.u $(LIBS-O3) -o $@

make_surface_bitlist: make_surface_bitlist.o
	$(CC) $(LFLAGS) make_surface_bitlist.o $(LIBS) -o $@

###########################################################################

create_label_map: create_label_map.o
	$(CC) $(LFLAGS) create_label_map.o $(LIBS) -o $@

###########################################################################

filter_volume: filter_volume.o
	$(CC) $(LFLAGS) filter_volume.o $(LIBS) -o $@

filter_volume-O3: filter_volume.u
	$(CC) -O3 $(LFLAGS) filter_volume.u $(LIBS-O3) -o $@

###########################################################################

fit_curve: fit_curve.o
	$(CC) $(LFLAGS) fit_curve.o $(LIBS) -o $@

fit_curve-O3: fit_curve.u
	$(CC) -O3 $(LFLAGS) fit_curve.u $(LIBS-O3 ) -o $@

###########################################################################

dump_transform: dump_transform.o
	$(CC) $(LFLAGS) dump_transform.o $(LIBS) -o $@

dump_transform-O3: dump_transform.u
	$(CC) -O3 $(LFLAGS) dump_transform.u $(LIBS-O3) -o $@

###########################################################################

print_axis_angles: print_axis_angles.o
	$(CC) $(LFLAGS) print_axis_angles.o $(LIBS) -o $@

print_axis_angles-O3: print_axis_angles.u
	$(CC) -O3 $(LFLAGS) print_axis_angles.u $(LIBS-O3) -o $@

###########################################################################

dump_polygons: dump_polygons.o
	$(CC) $(LFLAGS) dump_polygons.o $(LIBS) -o $@

###########################################################################

smooth_dilate_volume: smooth_dilate_volume.o
	$(CC) $(LFLAGS) smooth_dilate_volume.o $(LIBS) -o $@

smooth_dilate_volume-O3: smooth_dilate_volume.u
	$(CC) -O3 $(LFLAGS) smooth_dilate_volume.u $(LIBS-O3) -o $@

###########################################################################

dump_uv: dump_uv.o
	$(CC) $(LFLAGS) dump_uv.o $(LIBS) -o $@

###########################################################################

extract_tag_slice: extract_tag_slice.o
	$(CC) $(LFLAGS) extract_tag_slice.o $(LIBS) -o $@

###########################################################################

print_all_labels: print_all_labels.o
	$(CC) $(LFLAGS) print_all_labels.o $(LIBS) -o $@

###########################################################################

make_grid_lines: make_grid_lines.o
	$(CC) $(LFLAGS) make_grid_lines.o $(LIBS) -o $@

###########################################################################

place_images: place_images.o
	$(CC) $(LFLAGS) place_images.o $(LIBS) -o $@

###########################################################################

measure_surface_area: measure_surface_area.o
	$(CC) $(LFLAGS) measure_surface_area.o $(LIBS) -o $@

###########################################################################

scan_object_to_volume: scan_object_to_volume.o
	$(CC) $(LFLAGS) scan_object_to_volume.o $(LIBS) -o $@

scan_object_to_volume-O3: scan_object_to_volume.u
	$(CC) -O3 $(LFLAGS) scan_object_to_volume.u $(LIBS-O3) -o $@

###########################################################################

create_surface_interpolation_lsq: create_surface_interpolation_lsq.o
	$(CC) $(LFLAGS) create_surface_interpolation_lsq.o $(LIBS) -o $@

create_surface_interpolation_lsq-O3: create_surface_interpolation_lsq.u
	$(CC) -O3 $(LFLAGS) create_surface_interpolation_lsq.u $(LIBS-O3) -o $@

create_surface_interpolation: create_surface_interpolation.o
	$(CC) $(LFLAGS) create_surface_interpolation.o $(LIBS) -o $@

create_surface_interpolation-O3: create_surface_interpolation.u
	$(CC) -O3 $(LFLAGS) create_surface_interpolation.u $(LIBS-O3) -o $@

###########################################################################

flatten_polygons: flatten_polygons.o
	$(CC) $(LFLAGS) flatten_polygons.o $(LIBS) -o $@

flatten_polygons-O3: flatten_polygons.u
	$(CC) -O3 $(LFLAGS) flatten_polygons.u $(LIBS-O3) -o $@

flatten_polygons.pixie: flatten_polygons
	pixie -o $@ flatten_polygons


###########################################################################

create_2d_surface: create_2d_surface.o
	$(CC) $(LFLAGS) create_2d_surface.o $(LIBS) -o $@

create_2d_surface-O3: create_2d_surface.u
	$(CC) -O3 $(LFLAGS) create_2d_surface.u $(LIBS-O3 ) -o $@

prof.2d: create_2d_surface.pixie Makefile
	prof -quit 100 -pixie -h -p create_2d_surface >&! $@

prof.2da: create_2d_surface.pixie Makefile
	prof -O get_horizontal_coord -quit 10000 -pixie -h -p create_2d_surface >&! $@


###########################################################################

subdivide_values: subdivide_values.o
	$(CC) $(LFLAGS) subdivide_values.o $(LIBS) -o $@

###########################################################################

make_sphere_transform: make_sphere_transform.o
	$(CC) $(LFLAGS) make_sphere_transform.o $(LIBS) -o $@

###########################################################################

apply_sphere_transform: apply_sphere_transform.o
	$(CC) $(LFLAGS) apply_sphere_transform.o $(LIBS) -o $@

###########################################################################

two_surface_resample: two_surface_resample.o
	$(CC) $(LFLAGS) two_surface_resample.o $(LIBS) -o $@

two_surface_resample-O3: two_surface_resample.u
	$(CC) -O3 $(LFLAGS) two_surface_resample.u $(LIBS-O3) -o $@

###########################################################################

spherical_resample: spherical_resample.o
	$(CC) $(LFLAGS) spherical_resample.o $(LIBS) -o $@

spherical_resample-O3: spherical_resample.u
	$(CC) -O3 $(LFLAGS) spherical_resample.u $(LIBS-O3) -o $@

###########################################################################

flatten_sheet: flatten_sheet.o
	$(CC) $(LFLAGS) flatten_sheet.o $(LIBS) -o $@

flatten_sheet2: flatten_sheet2.o
	$(CC) $(LFLAGS) flatten_sheet2.o $(LIBS) -o $@

flatten_sheet2-O3: flatten_sheet2.u
	$(CC) -O3 $(LFLAGS) flatten_sheet2.u $(LIBS-O3) -o $@

flatten_sheet-O3: flatten_sheet.u
	$(CC) -O3 $(LFLAGS) flatten_sheet.u $(LIBS-O3) -o $@

prof.flat: flatten_sheet.pixie Makefile
	prof -quit 100 -pixie -h -p flatten_sheet >&! $@

###########################################################################

flatten_sheet3: flatten_sheet3.o
	$(CC) $(LFLAGS) flatten_sheet3.o $(LIBS) -o $@

flatten_sheet3-O3: flatten_sheet3.u
	$(CC) -O3 $(LFLAGS) flatten_sheet3.u $(LIBS-O3) -o $@

prof.fs3: flatten_sheet3-O3.pixie Makefile
	prof -quit 100 -pixie -h -p flatten_sheet3-O3 >&! $@

###########################################################################

flatten_to_sphere: flatten_to_sphere.o
	$(CC) $(LFLAGS) flatten_to_sphere.o $(LIBS) -o $@

flatten_to_sphere-O3: flatten_to_sphere.u
	$(CC) -O3 $(LFLAGS) flatten_to_sphere.u $(LIBS-O3) -o $@

###########################################################################

flatten_to_sphere2: flatten_to_sphere2.o
	$(CC) $(LFLAGS) flatten_to_sphere2.o $(LIBS) -o $@

flatten_to_sphere2-O3: flatten_to_sphere2.u
	$(CC) -O3 $(LFLAGS) flatten_to_sphere2.u $(LIBS-O3) -o $@

prof.flat2: flatten_to_sphere2.pixie flatten_to_sphere2.Counts Makefile
	prof -quit 100 -pixie -h -p flatten_to_sphere2 >&! $@

###########################################################################

flatten_to_sphere3: flatten_to_sphere3.o
	$(CC) $(LFLAGS) flatten_to_sphere3.o $(LIBS) -o $@

flatten_to_sphere3-O3: flatten_to_sphere3.u
	$(CC) -O3 $(LFLAGS) flatten_to_sphere3.u $(LIBS-O3) -o $@

prof.flat3: flatten_to_sphere3.pixie Makefile
	prof -quit 100 -pixie -h -p flatten_to_sphere3 >&! $@

###########################################################################

flatten_to_sphere4: flatten_to_sphere4.o
	$(CC) $(LFLAGS) flatten_to_sphere4.o $(LIBS) -o $@

flatten_to_sphere4-O3: flatten_to_sphere4.u
	$(CC) -O3 $(LFLAGS) flatten_to_sphere4.u $(LIBS-O3) -o $@

###########################################################################

flatten_to_sphere5: flatten_to_sphere5.o
	$(CC) $(LFLAGS) flatten_to_sphere5.o $(LIBS) -o $@

flatten_to_sphere5-O3: flatten_to_sphere5.u
	$(CC) -O3 $(LFLAGS) flatten_to_sphere5.u $(LIBS-O3) -o $@

prof.flat5: flatten_to_sphere5.pixie Makefile
	prof -quit 100 -pixie -h -p flatten_to_sphere5 >&! $@
	prof -O evaluate_fit -l -pixie flatten_to_sphere5 >> $@

###########################################################################

flatten_to_sphere6: flatten_to_sphere6.o
	$(CC) $(LFLAGS) flatten_to_sphere6.o $(LIBS) -o $@

flatten_to_sphere6-O3: flatten_to_sphere6.u
	$(CC) -O3 $(LFLAGS) flatten_to_sphere6.u $(LIBS-O3) -o $@

prof.flat6: flatten_to_sphere6.pixie Makefile
	prof -quit 100 -pixie -h -p flatten_to_sphere6 >&! $@
	prof -O evaluate_fit -l -pixie flatten_to_sphere6 >> $@

###########################################################################

flatten_to_sphere7: flatten_to_sphere7.o
	$(CC) $(LFLAGS) flatten_to_sphere7.o $(LIBS) -o $@

flatten_to_sphere7-O3: flatten_to_sphere7.u
	$(CC) -O3 $(LFLAGS) flatten_to_sphere7.u $(LIBS-O3) -o $@

prof.flat7: flatten_to_sphere7.pixie Makefile
	prof -quit 100 -pixie -h -p flatten_to_sphere7 >&! $@
	prof -O evaluate_fit -l -pixie flatten_to_sphere7 >> $@

###########################################################################

flatten_to_sphere8: flatten_to_sphere8.o
	$(CC) $(LFLAGS) flatten_to_sphere8.o $(LIBS) -o $@

flatten_to_sphere8-O3: flatten_to_sphere8.u
	$(CC) -O3 $(LFLAGS) flatten_to_sphere8.u $(LIBS-O3) -o $@

prof.flat8: flatten_to_sphere8.pixie Makefile
	prof -quit 100 -pixie -h -p flatten_to_sphere8 >&! $@
	prof -O evaluate_fit -l -pixie flatten_to_sphere8 >> $@

###########################################################################

flatten_to_sphere9: flatten_to_sphere9.o
	$(CC) $(LFLAGS) flatten_to_sphere9.o $(LIBS) -o $@

flatten_to_sphere9-O3: flatten_to_sphere9.u
	$(CC) -O3 $(LFLAGS) flatten_to_sphere9.u $(LIBS-O3) -o $@

prof.flat9: flatten_to_sphere9.pixie Makefile
	prof -quit 100 -pixie -h -p flatten_to_sphere9 >&! $@
	prof -O evaluate_fit -l -pixie flatten_to_sphere9 >> $@

###########################################################################

fit_2d: fit_2d.o
	$(CC) $(LFLAGS) fit_2d.o $(LIBS) -o $@

fit_2d-O3: fit_2d.u
	$(CC) -O3 $(LFLAGS) fit_2d.u $(LIBS-O3) -o $@

###########################################################################

fit_3d: fit_3d.o
	$(CC) $(LFLAGS) fit_3d.o $(LIBS) -o $@

fit_3d-O3: fit_3d.u
	$(CC) -O3 $(LFLAGS) fit_3d.u $(LIBS-O3) -o $@

prof.3d: fit_3d.pixie Makefile
	prof -E MI_convert_type -E add_to_linear_least_squares \
             -E scaled_maximal_pivoting_gaussian_elimination \
              -quit 100 -pixie -h -p fit_3d >&! $@


###########################################################################

subsample_polygons: subsample_polygons.o
	$(CC) $(LFLAGS) subsample_polygons.o $(LIBS) -o $@

subsample_polygons-O3: subsample_polygons.u
	$(CC) -O3 $(LFLAGS) subsample_polygons.u $(LIBS-O3) -o $@

###########################################################################

dim_image: dim_image.o
	$(CC) $(LFLAGS) dim_image.o $(LIBS) -o $@

dim_image-O3: dim_image.u
	$(CC) -O3 $(LFLAGS) dim_image.u $(LIBS-O3) -o $@

###########################################################################

create_2d_sheet: create_2d_sheet.o
	$(CC) $(LFLAGS) create_2d_sheet.o $(LIBS) -o $@

create_2d_sheet-O3: create_2d_sheet.u
	$(CC) -O3 $(LFLAGS) create_2d_sheet.u $(LIBS-O3) -o $@

###########################################################################

create_warping_points: create_warping_points.o
	$(CC) $(LFLAGS) create_warping_points.o $(LIBS) -o $@

create_warping_points-O3: create_warping_points.u
	$(CC) $(LFLAGS) -O3 create_warping_points.u $(LIBS-O3) -o $@

###########################################################################

label_sulci: label_sulci.o
	$(CC) $(LFLAGS) label_sulci.o $(LIBS) -o $@

label_sulci-O3: label_sulci.u
	$(CC) $(LFLAGS) -O3 label_sulci.u $(LIBS-O3) -o $@

###########################################################################

split_polygons: split_polygons.o
	$(CC) $(LFLAGS) split_polygons.o $(LIBS) -o $@

split_polygons-O3: split_polygons.u
	$(CC) -O3 $(LFLAGS) split_polygons.u $(LIBS-O3) -o $@

###########################################################################

merge_polygons: merge_polygons.o
	$(CC) $(LFLAGS) merge_polygons.o $(LIBS) -o $@

###########################################################################

tristrip_polygons: tristrip_polygons.o
	$(CC) $(LFLAGS) tristrip_polygons.o $(LIBS) -o $@

###########################################################################

fill_sulci: fill_sulci.o
	$(CC) $(LFLAGS) fill_sulci.o $(LIBS) -o $@

fill_sulci-O3: fill_sulci.u
	$(CC) -O3 $(LFLAGS) fill_sulci.u $(LIBS-O3) -o $@

###########################################################################

check_polygons: check_polygons.o
	$(CC) $(LFLAGS) check_polygons.o $(LIBS) -o $@

###########################################################################

separate_polygons: separate_polygons.o
	$(CC) $(LFLAGS) separate_polygons.o $(LIBS) -o $@

###########################################################################

create_marching_coords: create_marching_coords.o
	$(CC) $(LFLAGS) create_marching_coords.o $(LIBS) -o $@

create_marching_coords-O3: create_marching_coords.u
	$(CC) -O3 $(LFLAGS) create_marching_coords.u $(LIBS-O3) -o $@

###########################################################################

embed_in_sphere: embed_in_sphere.o
	$(CC) $(LFLAGS) embed_in_sphere.o $(LIBS) -o $@

embed_in_sphere-O3: embed_in_sphere.u
	$(CC) -O3 $(LFLAGS) embed_in_sphere.u $(LIBS-O3) -o $@

###########################################################################

open_manifold: open_manifold.o
	$(CC) $(LFLAGS) open_manifold.o $(LIBS) -o $@

open_manifold-O3: open_manifold.u
	$(CC) -O3 $(LFLAGS) open_manifold.u $(LIBS-O3) -o $@

###########################################################################

segment_polygons: segment_polygons.o
	$(CC) $(LFLAGS) segment_polygons.o $(LIBS) -o $@

segment_polygons-O3: segment_polygons.u
	$(CC) -O3 $(LFLAGS) segment_polygons.u $(LIBS-O3) -o $@

###########################################################################

manifold_polygons: manifold_polygons.o
	$(CC) $(LFLAGS) manifold_polygons.o $(LIBS) -o $@

manifold_polygons-O3: manifold_polygons.u
	$(CC) -O3 $(LFLAGS) manifold_polygons.u $(LIBS-O3) -o $@

###########################################################################

manifold_polygons2: manifold_polygons2.o
	$(CC) $(LFLAGS) manifold_polygons2.o $(LIBS) -o $@

manifold_polygons2-O3: manifold_polygons2.u
	$(CC) -O3 $(LFLAGS) manifold_polygons2.u $(LIBS-O3) -o $@

###########################################################################

manifold_labels: manifold_labels.o
	$(CC) $(LFLAGS) manifold_labels.o $(LIBS) -o $@

manifold_labels-O3: manifold_labels.u
	$(CC) -O3 $(LFLAGS) manifold_labels.u $(LIBS-O3) -o $@

###########################################################################

map_sheets: map_sheets.o
	$(CC) $(LFLAGS) map_sheets.o $(LIBS) -o $@

map_sheets-O3: map_sheets.u
	$(CC) -O3 $(LFLAGS) map_sheets.u $(LIBS-O3) -o $@

###########################################################################

evaluate_spline: evaluate_spline.o evaluate_basis.o cubic_approximation.o
	$(CC) $(LFLAGS) evaluate_spline.o evaluate_basis.o \
                        cubic_approximation.o $(LIBS) -o $@

###########################################################################

interpolate_tags: interpolate_tags.o
	$(CC) $(LFLAGS) interpolate_tags.o $(LIBS) -o $@

###########################################################################

EXTRA_INC = -I/usr/people/neelin/src/minc/progs/Proglib
EXTRA_L = -L /usr/people/neelin/src/minc/progs/Proglib

mincresample.o resample_volumes.o:
	$(CC) -c $(CFLAGS) $(EXTRA_INC) $<

mincresample.u resample_volumes.u:
	$(CC) -c -O3 $(CFLAGS) $(EXTRA_INC) $< -j

mincresample: mincresample.o resample_volumes.o thin_plate_spline.o
	$(CC) $(LFLAGS) mincresample.o resample_volumes.o thin_plate_spline.o \
                           $(EXTRA_L) -lmincprog $(LIBS) -o $@

mincresample-O3: mincresample.u resample_volumes.u thin_plate_spline.u
	$(CC) -O3 $(LFLAGS) mincresample.u resample_volumes.u thin_plate_spline.u \
                           $(EXTRA_L) -lmincprog $(LIBS-O3) -o $@

mincresample.pixie: mincresample
	pixie -o $@ mincresample

mincresample-O3.pixie: mincresample-O3
	pixie -o $@ mincresample-O3

prof.mincresample: mincresample.pixie Makefile
	prof -quit 100 -pixie -h -p mincresample >&! $@

prof.mincresample-O3: mincresample-O3.pixie Makefile
	prof -quit 100 -pixie -h -p mincresample-O3 >&! $@

###########################################################################

reconstitute_points: reconstitute_points.o
	$(CC) $(LFLAGS) reconstitute_points.o $(LIBS) -o $@

###########################################################################

scan_lines_to_polygons: scan_lines_to_polygons.o
	$(CC) $(LFLAGS) scan_lines_to_polygons.o $(LIBS) -o $@

scan_lines_to_polygons-O3: scan_lines_to_polygons.u
	$(CC) -O3 $(LFLAGS) scan_lines_to_polygons.u $(LIBS-O3) -o $@

###########################################################################

segment_probabilities: segment_probabilities.o
	$(CC) $(LFLAGS) segment_probabilities.o $(LIBS) -o $@

###########################################################################

classify_sulcus: classify_sulcus.o
	$(CC) $(LFLAGS) classify_sulcus.o $(LIBS) -o $@

classify_sulcus-O3: classify_sulcus.u
	$(CC) $(LFLAGS) classify_sulcus.u -O3 $(LIBS-O3) -o $@

###########################################################################

test_point_polygon_distance: test_point_polygon_distance.o
	$(CC) $(LFLAGS) test_point_polygon_distance.o $(LIBS) -o $@

test_point_polygon_distance-O3: test_point_polygon_distance.u
	$(CC) $(LFLAGS) test_point_polygon_distance.u -O3 $(LIBS-O3) -o $@

prof.dist: test_point_polygon_distance.pixie Makefile
	prof -quit 100 -pixie -h -p test_point_polygon_distance >&! $@

###########################################################################

average_objects: average_objects.o
	$(CC) $(LFLAGS) average_objects.o $(LIBS) -o $@

###########################################################################

dilate_volume_completely: dilate_volume_completely.o
	$(CC) $(LFLAGS) dilate_volume_completely.o $(LIBS) -o $@

###########################################################################

composite_volumes: composite_volumes.o
	$(CC) $(LFLAGS) composite_volumes.o $(LIBS) -o $@

composite_volumes-O3: composite_volumes.u
	$(CC) $(LFLAGS) composite_volumes.u -O3 $(LIBS-O3) -o $@

###########################################################################

arbitrary_subdivide: arbitrary_subdivide.o
	$(CC) $(LFLAGS) arbitrary_subdivide.o $(LIBS) -o $@

arbitrary_subdivide-O3: arbitrary_subdivide.u
	$(CC) $(LFLAGS) arbitrary_subdivide.u -O3 $(LIBS-O3) -o $@

###########################################################################

resample_triangulation: resample_triangulation.o
	$(CC) $(LFLAGS) resample_triangulation.o $(LIBS) -o $@

resample_triangulation-O3: resample_triangulation.u
	$(CC) $(LFLAGS) resample_triangulation.u -O3 $(LIBS-O3) -o $@

resample_triangulation.pixie: resample_triangulation
	pixie -o $@ resample_triangulation

prof.res: resample_triangulation.Counts
	prof -quit 100 -pixie -h -p resample_triangulation >&! $@

###########################################################################

make_max_grad: make_max_grad.o
	$(CC) $(LFLAGS) make_max_grad.o $(LIBS) -o $@

make_max_grad-O3: make_max_grad.u
	$(CC) $(LFLAGS) make_max_grad.u -O3 $(LIBS-O3) -o $@

###########################################################################

mask_values: mask_values.o
	$(CC) $(LFLAGS) mask_values.o $(LIBS) -o $@

###########################################################################

create_function_volume: create_function_volume.o
	$(CC) $(LFLAGS) create_function_volume.o $(LIBS) -o $@

###########################################################################

delauney: delauney.o
	$(CC) $(LFLAGS) delauney.o $(LIBS) -o $@

###########################################################################

trimesh_to_polygons: trimesh_to_polygons.o
	$(CC) $(LFLAGS) trimesh_to_polygons.o $(LIBS) -o $@

###########################################################################

trimesh_resample: trimesh_resample.o
	$(CC) $(LFLAGS) trimesh_resample.o $(LIBS) -o $@

trimesh_resample.pixie: trimesh_resample
	pixie -o $@ trimesh_resample

prof.tri: trimesh_resample.Counts
	prof -quit 100 -pixie -h -p trimesh_resample >&! $@

###########################################################################

trimesh_set_points: trimesh_set_points.o
	$(CC) $(LFLAGS) trimesh_set_points.o $(LIBS) -o $@

###########################################################################

test_label_volume: test_label_volume.o labels.o
	$(CC) $(LFLAGS) test_label_volume.o labels.o $(LIBS) -o $@

test_label_volume.pixie: test_label_volume
	pixie -o $@ test_label_volume

prof.label: test_label_volume.Counts
	prof -quit 100 -pixie -h -p test_label_volume >&! $@

###########################################################################

make_diff_volume: make_diff_volume.o
	$(CC) $(LFLAGS) make_diff_volume.o $(LIBS) -o $@

###########################################################################

t_stat_to_probability: t_stat_to_probability.o
	$(CC) $(LFLAGS) t_stat_to_probability.o $(LIBS) -o $@

###########################################################################

dump_vertex: dump_vertex.o
	$(CC) $(LFLAGS) dump_vertex.o $(LIBS) -o $@

###########################################################################

extract_largest_line: extract_largest_line.o
	$(CC) $(LFLAGS) extract_largest_line.o $(LIBS) -o $@

###########################################################################

preprocess_segmentation: preprocess_segmentation.o
	$(CC) $(LFLAGS) preprocess_segmentation.o $(LIBS) -o $@

###########################################################################

reparameterize_line: reparameterize_line.o
	$(CC) $(LFLAGS) reparameterize_line.o $(LIBS) -o $@

###########################################################################

equalize_distance: equalize_distance.o
	$(CC) $(LFLAGS) equalize_distance.o $(LIBS) -o $@

equalize_distance2: equalize_distance2.o
	$(CC) $(LFLAGS) equalize_distance2.o $(LIBS) -o $@

equalize_distance3: equalize_distance3.o
	$(CC) $(LFLAGS) equalize_distance3.o $(LIBS) -o $@

###########################################################################

print_object_limits: print_object_limits.o
	$(CC) $(LFLAGS) print_object_limits.o $(LIBS) -o $@

###########################################################################

chamfer_volume: chamfer_volume.o
	$(CC) $(LFLAGS) chamfer_volume.o $(LIBS) -o $@

###########################################################################

prime: prime.o
	$(CC) $(LFLAGS) prime.o $(LIBS) -o $@

###########################################################################

refine_mesh: refine_mesh.o
	$(CC) $(LFLAGS) refine_mesh.o $(LIBS) -o $@

###########################################################################

solve_view: solve_view.o
	$(CC) $(LFLAGS) solve_view.o $(LIBS) -o $@

###########################################################################

minc_to_rgb: minc_to_rgb.o
	$(CC) $(LFLAGS) minc_to_rgb.o $(LIBS) -o $@

###########################################################################

mask_volume_4d: mask_volume_4d.o
	$(CC) $(LFLAGS) mask_volume_4d.o $(LIBS) -o $@

###########################################################################

compare_lengths: compare_lengths.o
	$(CC) $(LFLAGS) compare_lengths.o $(LIBS) -o $@

###########################################################################

create_sphere_volume: create_sphere_volume.o
	$(CC) $(LFLAGS) create_sphere_volume.o $(LIBS) -o $@

###########################################################################

composite_images: composite_images.o
	$(CC) $(LFLAGS) composite_images.o $(LIBS) -o $@

###########################################################################

composite_minc_images: composite_minc_images.o
	$(CC) $(LFLAGS) composite_minc_images.o $(LIBS) -o $@

###########################################################################

grid_transform: grid_transform.o
	$(CC) $(LFLAGS) grid_transform.o $(LIBS) -o $@

###########################################################################

print_2d_coords: print_2d_coords.o
	$(CC) $(LFLAGS) print_2d_coords.o $(LIBS) -o $@

###########################################################################

scale_minc_image: scale_minc_image.o
	$(CC) $(LFLAGS) scale_minc_image.o $(LIBS) -o $@

###########################################################################

make_columns: make_columns.o
	$(CC) $(LFLAGS) make_columns.o $(LIBS) -o $@

###########################################################################

distance_surfaces: distance_surfaces.o
	$(CC) $(LFLAGS) distance_surfaces.o $(LIBS) -o $@

###########################################################################

print_world_value: print_world_value.o
	$(CC) $(LFLAGS) print_world_value.o $(LIBS) -o $@

###########################################################################

cross_correlation_3d: cross_correlation_3d.o
	$(CC) $(LFLAGS) cross_correlation_3d.o $(LIBS) -o $@

###########################################################################

image_io: image_io.o
	$(CC) $(LFLAGS) image_io.o $(LIBS) -o $@

###########################################################################

conjugate_grad_prototypes.h: conjugate_grad.c
	@echo "#ifndef  DEF_CONJUGATE_GRAD_PROTOTYPES"                 >  $@
	@echo "#define  DEF_CONJUGATE_GRAD_PROTOTYPES"                 >> $@
	@extract_functions -public conjugate_grad.c                    >> $@
	@echo "#endif"                                                 >> $@

conjugate_min_prototypes.h: conjugate_min.c
	@echo "#ifndef  DEF_CONJUGATE_MIN_PROTOTYPES"                  >  $@
	@echo "#define  DEF_CONJUGATE_MIN_PROTOTYPES"                  >> $@
	@extract_functions -public conjugate_min.c                     >> $@
	@echo "#endif"                                                 >> $@

line_min_prototypes.h: line_minimization.c
	@echo "#ifndef  DEF_LINE_MINIMIZATION_PROTOTYPES"              >  $@
	@echo "#define  DEF_LINE_MINIMIZATION_PROTOTYPES"              >> $@
	@extract_functions -public line_minimization.c                 >> $@
	@echo "#endif"                                                 >> $@

test_conjugate: conjugate_grad_prototypes.h conjugate_min_prototypes.h \
                line_min_prototypes.h \
                test_conjugate.o conjugate_min.o conjugate_grad.o \
                line_minimization.o
	$(CC) $(LFLAGS) test_conjugate.o conjugate_min.o conjugate_grad.o \
                        line_minimization.o $(LIBS) -o $@

test_new_min: test_new_min.o
	$(CC) $(LFLAGS) test_new_min.o $(LIBS) -o $@

fit_volumes: conjugate_grad_prototypes.h conjugate_min_prototypes.h \
                line_min_prototypes.h \
                fit_volumes.o conjugate_min.o conjugate_grad.o \
                line_minimization.o
	$(CC) $(LFLAGS) fit_volumes.o conjugate_min.o conjugate_grad.o \
                        line_minimization.o $(LIBS) -o $@

###########################################################################

test_hyperslab: test_hyperslab.o
	$(CC) $(LFLAGS) test_hyperslab.o $(LIBS) -o $@

###########################################################################

split_file: split_file.o
	$(CC) $(LFLAGS) split_file.o $(LIBS) -o $@

###########################################################################

compute_bounding_view: compute_bounding_view.o
	$(CC) $(LFLAGS) compute_bounding_view.o $(LIBS) -o $@

###########################################################################

fit_curve2: fit_curve2.o  conjugate_min.o conjugate_grad.o \
                line_minimization.o
	$(CC) $(LFLAGS) fit_curve2.o  conjugate_min.o conjugate_grad.o \
                line_minimization.o $(LIBS) -o $@

###########################################################################

test: test.o
	$(CC) $(LFLAGS) test.o $(LIBS) -o $@

###########################################################################

gaussian_blur_peaks: gaussian_blur_peaks.o
	$(CC) $(LFLAGS) gaussian_blur_peaks.o $(LIBS) -o $@

###########################################################################

test_c: test_c.o
	$(CC) $(LFLAGS) test_c.o $(LIBS) -o $@

###########################################################################

reorder_volume: reorder_volume.o
	$(CC) $(LFLAGS) reorder_volume.o $(LIBS) -o $@

###########################################################################

make_line_links: make_line_links.o
	$(CC) $(LFLAGS) make_line_links.o $(LIBS) -o $@

###########################################################################

rf_volume: rf_volume.o
	$(CC) $(LFLAGS) rf_volume.o $(LIBS) -o $@

###########################################################################

solve_camera: solve_camera.o
	$(CC) $(LFLAGS) solve_camera.o $(LIBS) -o $@

###########################################################################

offset_volume: offset_volume.o
	$(CC) $(LFLAGS) offset_volume.o $(LIBS) -o $@

###########################################################################

offset_volume2: offset_volume2.o
	$(CC) $(LFLAGS) offset_volume2.o $(LIBS) -o $@

###########################################################################

find_image_bounding_box: find_image_bounding_box.o
	$(CC) $(LFLAGS) find_image_bounding_box.o $(LIBS) -o $@

###########################################################################

test_deriv: test_deriv.o
	$(CC) $(LFLAGS) test_deriv.o $(LIBS) -o $@

###########################################################################

test_smooth: conjugate_grad_prototypes.h conjugate_min_prototypes.h \
                line_min_prototypes.h \
                test_smooth.o conjugate_min.o conjugate_grad.o \
                line_minimization.o
	$(CC) $(LFLAGS) test_smooth.o conjugate_min.o conjugate_grad.o \
                        line_minimization.o $(LIBS) -o $@

###########################################################################

gradient_histogram: gradient_histogram.o
	$(CC) $(LFLAGS) gradient_histogram.o $(LIBS) -o $@

###########################################################################

print_all_label_bounding_boxes: print_all_label_bounding_boxes.o
	$(CC) $(LFLAGS) print_all_label_bounding_boxes.o $(LIBS) -o $@

###########################################################################

find_buried_surface: find_buried_surface.o
	$(CC) $(LFLAGS) find_buried_surface.o $(LIBS) -o $@

###########################################################################

find_buried_surface2: find_buried_surface2.o
	$(CC) $(LFLAGS) find_buried_surface2.o $(LIBS) -o $@

find_buried_surface2.pixie: find_buried_surface2
	pixie -o $@ find_buried_surface2

prof.buried: find_buried_surface2.Counts
	prof -quit 100 -pixie -h -p find_buried_surface2 >&! $@

###########################################################################

tags_to_spheres: tags_to_spheres.o
	$(CC) $(LFLAGS) tags_to_spheres.o $(LIBS) -o $@

###########################################################################

print_object_centroid: print_object_centroid.o
	$(CC) $(LFLAGS) print_object_centroid.o $(LIBS) -o $@

###########################################################################

expand_polygons_slightly: expand_polygons_slightly.o
	$(CC) $(LFLAGS) expand_polygons_slightly.o $(LIBS) -o $@

###########################################################################

distance_surfaces_haus: distance_surfaces_haus.o
	$(CC) $(LFLAGS) distance_surfaces_haus.o $(LIBS) -o $@

###########################################################################
